#!/usr/bin/env python3
"""
Validacao do TP2 por implementacao de referencia independente.

Para cada instancia, executa o programa em C (./tp2) nas opcoes do menu,
le a saida e compara com o resultado calculado aqui em Python
(NetworkX / SciPy), que nao compartilha codigo com a biblioteca em C.

Uso:  python3 validar.py [--tempo-milp SEGUNDOS]
Gera: saidas/<instancia>_<opcao>.txt  (transcricoes limpas)
      resultados.json                  (numeros usados no relatorio)
"""
import json
import math
import os
import re
import statistics
import subprocess
import sys

import networkx as nx
import numpy as np
from scipy.optimize import milp, LinearConstraint, Bounds
from scipy.sparse import csr_matrix, lil_matrix
from scipy.sparse.csgraph import dijkstra

INSTANCIAS = [
    ("01_pequeno_euleriano.txt", "instancia1"),
    ("02_estrutural.txt", "instancia2"),
    ("03_esparso.txt", "instancia3"),
    ("04_denso.txt", "instancia4"),
]
REPETICOES = 7          # execucoes para medir tempo (mediana)
TEMPO_MILP = 120        # segundos para o otimo exato da cobertura (instancia grande)
if "--tempo-milp" in sys.argv:
    TEMPO_MILP = float(sys.argv[sys.argv.index("--tempo-milp") + 1])

MENU = re.compile(r"\n={40}\nANALISE DE REDE SOCIAL\n={40}\n(?:\d+\s+- [^\n]*\n)+0  - Sair\n")


# ----------------------------------------------------------------------------
# execucao do programa em C
# ----------------------------------------------------------------------------
def rodar(arquivo, opcao, vertice=None):
    entrada = f"{opcao}\n" + (f"{vertice}\n" if vertice is not None else "") + "0\n"
    p = subprocess.run(["./tp2", arquivo], input=entrada, capture_output=True,
                       text=True, timeout=600)
    texto = MENU.sub("\n", p.stdout)
    # tudo entre o primeiro prompt e o fim do programa
    texto = texto.split("Digite a opcao: ", 1)[1]
    texto = texto.replace("Informe o vertice: ", "")
    texto = texto.split("Digite a opcao: Programa encerrado.")[0]
    texto = texto.replace("Programa encerrado.", "").strip("\n")
    texto = re.sub(r"Digite a opcao: ?$", "", texto).rstrip()
    return texto


def tempos(texto):
    return [float(x) for x in re.findall(r"Tempo: ([\d.]+) ms", texto)]


# ----------------------------------------------------------------------------
# leitura do grafo (referencia)
# ----------------------------------------------------------------------------
def ler(arquivo):
    linhas = [l for l in open(arquivo).read().split("\n") if l.strip()]
    n = int(linhas[0])
    G = nx.Graph()
    G.add_nodes_from(range(1, n + 1))
    for l in linhas[1:]:
        u, v, w = l.split()
        G.add_edge(int(u), int(v), weight=float(w))
    return n, G


# ----------------------------------------------------------------------------
# replicas dos algoritmos de cobertura (mesmas regras de desempate do C)
# ----------------------------------------------------------------------------
def ref_gulosa(n, G):
    adj = {v: set(G[v]) for v in G}
    grau = {v: len(adj[v]) for v in G}
    esc = set()
    while True:
        melhor = -1
        for v in range(1, n + 1):
            if v not in esc and grau[v] > 0 and (melhor == -1 or grau[v] > grau[melhor]):
                melhor = v
        if melhor == -1:
            break
        esc.add(melhor)
        for v in adj[melhor]:
            if v not in esc:
                grau[v] -= 1
        grau[melhor] = 0
    return sorted(esc)


def ref_heuristica(n, G):
    esc = set()
    for u, v in sorted((min(a, b), max(a, b)) for a, b in G.edges()):
        if u not in esc and v not in esc:
            esc.add(u)
            esc.add(v)
    for u in range(1, n + 1):
        if u in esc and all(v in esc for v in G[u]):
            esc.discard(u)
    return sorted(esc)


def cobertura_valida(G, esc):
    s = set(esc)
    return all(u in s or v in s for u, v in G.edges())


def cobertura_otima(n, G, limite):
    """Cobertura minima exata via programacao inteira (HiGHS). Retorna
    (melhor_encontrada, limite_inferior_dual, provada_otima)."""
    arestas = list(G.edges())
    A = lil_matrix((len(arestas), n))
    for i, (u, v) in enumerate(arestas):
        A[i, u - 1] = 1
        A[i, v - 1] = 1
    res = milp(c=np.ones(n), constraints=LinearConstraint(csr_matrix(A), lb=1, ub=np.inf),
               integrality=np.ones(n), bounds=Bounds(0, 1),
               options={"time_limit": limite, "disp": False})
    melhor = int(round(res.fun)) if res.x is not None else None
    dual = getattr(res, "mip_dual_bound", None)
    provada = (res.status == 0)
    return melhor, (None if dual is None else float(dual)), provada


def cobertura_busca_aleatoria(n, G, tentativas=4000, semente=1):
    """Limite SUPERIOR para a cobertura minima: n - |conjunto independente|.
    Guloso de grau minimo com desempate aleatorio, repetido varias vezes
    (conjuntos em bitset). Nao prova otimalidade; so da uma referencia melhor."""
    import random
    rng = random.Random(semente)
    adj = [0] * n
    for u, v in G.edges():
        adj[u - 1] |= 1 << (v - 1)
        adj[v - 1] |= 1 << (u - 1)
    melhor = 0
    for _ in range(tentativas):
        restante = (1 << n) - 1
        tam = 0
        while restante:
            cands, menor = [], n + 1
            r = restante
            while r:
                b = r & -r
                v = b.bit_length() - 1
                r ^= b
                g = (adj[v] & restante).bit_count()
                if g < menor:
                    menor, cands = g, [v]
                elif g == menor:
                    cands.append(v)
            v = rng.choice(cands)
            tam += 1
            restante &= ~(adj[v] | (1 << v))
        melhor = max(melhor, tam)
    return n - melhor


# ----------------------------------------------------------------------------
# parsing das saidas
# ----------------------------------------------------------------------------
def parse_arestas_arvore(texto):
    return [(int(a), int(b), float(w))
            for a, b, w in re.findall(r"\((\d+), (\d+)\), peso ([\d.]+)", texto)]


def lista_apos(rotulo, texto):
    m = re.search(rotulo + r"[^:\n]*:([^\n]*)", texto)
    return [int(x) for x in m.group(1).split()] if m else []


# ----------------------------------------------------------------------------
checks = []   # (instancia, nome, ok, detalhe)


def conferir(inst, nome, ok, detalhe=""):
    checks.append((inst, nome, bool(ok), detalhe))
    print(f"  [{'OK ' if ok else 'ERRO'}] {nome} {detalhe}")


def main():
    os.makedirs("saidas", exist_ok=True)
    resultados = {}
    for arquivo, rotulo in INSTANCIAS:
        print(f"\n=== {arquivo} ===")
        n, G = ler(arquivo)
        m = G.number_of_edges()
        R = {"arquivo": arquivo, "n": n, "m": m}
        transcricao = []

        def exec_opcao(op, descricao):
            t = rodar(arquivo, op)
            transcricao.append(f"### Opcao {op} - {descricao}\n{t}\n")
            return t

        # ---- 1, 2, 3 : ordem, tamanho, densidade ----
        t1 = exec_opcao(1, "ordem")
        t2 = exec_opcao(2, "tamanho")
        t3 = exec_opcao(3, "densidade")
        conferir(rotulo, "ordem", f"{n} vertices" in t1)
        conferir(rotulo, "tamanho", f"{m} arestas" in t2)
        dens = m / n
        dnorm = 2 * m / (n * (n - 1))
        conferir(rotulo, "densidade m/n e normalizada",
                 f"{dens:.6f}" in t3 and f"{dnorm:.6f}" in t3)
        R.update(densidade=dens, densidade_normalizada=dnorm)

        # ---- 8 : componentes ; 9 : ciclo ----
        t8 = exec_opcao(8, "componentes conexas")
        c = nx.number_connected_components(G)
        conferir(rotulo, "componentes", f"conexas: {c}" in t8, f"({c})")
        t9 = exec_opcao(9, "ciclos")
        tem_ciclo = m > n - c
        conferir(rotulo, "ciclo", ("possui ciclo" in t9 and "nao possui" not in t9) == tem_ciclo)
        R["conexo"] = (c == 1)
        graus = [d for _, d in G.degree()]
        R["grau_min"], R["grau_max"], R["grau_medio"] = min(graus), max(graus), sum(graus) / n
        R["vertices_grau_impar"] = sorted(v for v, d in G.degree() if d % 2)

        # ---- 11 : circuito Euleriano ----
        t11 = exec_opcao(11, "circuito Euleriano")
        existe = nx.is_eulerian(G)
        R["euleriano"] = existe
        if existe:
            ok_exist = "possui circuito Euleriano." in t11 and "nao possui" not in t11
            conferir(rotulo, "Euleriano: existencia", ok_exist, "(existe)")
            seq = [int(x) for x in re.search(r"arestas\):\n([^\n]*)", t11).group(1).split("->")]
            usadas = [frozenset(p) for p in zip(seq, seq[1:])]
            valido = (seq[0] == seq[-1] and len(seq) == m + 1
                      and len(set(usadas)) == m
                      and all(G.has_edge(*tuple(a)) for a in usadas))
            conferir(rotulo, "Euleriano: circuito valido (fecha, m arestas, cada uma 1x)", valido)
            R["circuito_tamanho"] = len(seq) - 1
        else:
            conferir(rotulo, "Euleriano: inexistencia", "nao possui circuito Euleriano" in t11, "(nao existe)")
            impares = lista_apos("Vertices de grau impar", t11)
            conferir(rotulo, "Euleriano: vertices de grau impar",
                     impares == R["vertices_grau_impar"], f"({len(impares)} vertices)")

        # ---- 12 : AGM ----
        if c == 1:
            t12 = exec_opcao(12, "arvore geradora minima")
            T = nx.minimum_spanning_tree(G, algorithm="kruskal")
            custo_ref = T.size(weight="weight")
            arv = parse_arestas_arvore(t12)
            custo_c = float(re.search(r"Custo total: ([\d.]+)", t12).group(1))
            H = nx.Graph()
            H.add_nodes_from(range(1, n + 1))
            for a, b, w in arv:
                H.add_edge(a, b, weight=w)
            e_arvore = (len(arv) == n - 1 and nx.is_connected(H)
                        and all(G.has_edge(a, b) and abs(G[a][b]["weight"] - w) < 1e-9 for a, b, w in arv))
            conferir(rotulo, "AGM: e arvore geradora (n-1 arestas, conexa, arestas do grafo)", e_arvore)
            conferir(rotulo, "AGM: custo igual ao de Kruskal", abs(custo_ref - custo_c) < 0.006 * 1,
                     f"(C={custo_c:.2f}, ref={custo_ref:.2f})")
            pesos_unicos = len({d["weight"] for *_, d in G.edges(data=True)}) == m
            if pesos_unicos:
                a_c = {frozenset((a, b)) for a, b, _ in arv}
                a_r = {frozenset(e) for e in T.edges()}
                conferir(rotulo, "AGM: mesmo conjunto de arestas (pesos distintos => AGM unica)", a_c == a_r)
            R["agm_custo"] = custo_ref
            R["agm_arestas"] = len(arv)
            R["pesos_distintos"] = pesos_unicos
        else:
            R["agm_custo"] = None

        # ---- 13 : excentricidade, raio, diametro ----
        if c == 1:
            t13 = exec_opcao(13, "excentricidade, raio e diametro")
            M = np.zeros((n, n))
            for u, v, d in G.edges(data=True):
                M[u - 1, v - 1] = M[v - 1, u - 1] = d["weight"]
            D = dijkstra(csr_matrix(M), directed=False)
            ecc = D.max(axis=1)
            ecc_c = [float(x) for x in re.findall(r"^\d+: ([\d.]+)$", t13, re.M)]
            conferir(rotulo, "metricas: excentricidades",
                     len(ecc_c) == n and np.max(np.abs(np.array(ecc_c) - ecc)) < 0.006)
            raio, diam = float(ecc.min()), float(ecc.max())
            raio_c = float(re.search(r"Raio: ([\d.]+)", t13).group(1))
            diam_c = float(re.search(r"Diametro: ([\d.]+)", t13).group(1))
            conferir(rotulo, "metricas: raio e diametro",
                     abs(raio - raio_c) < 0.006 and abs(diam - diam_c) < 0.006,
                     f"(raio={raio_c:.2f}, diametro={diam_c:.2f})")
            centro_ref = [i + 1 for i in range(n) if abs(ecc[i] - raio) < 1e-9]
            perif_ref = [i + 1 for i in range(n) if abs(ecc[i] - diam) < 1e-9]
            conferir(rotulo, "metricas: centro e periferia",
                     lista_apos("Centro", t13) == centro_ref and lista_apos("Periferia", t13) == perif_ref,
                     f"(centro={centro_ref[:6]}{'...' if len(centro_ref) > 6 else ''}, "
                     f"periferia={perif_ref[:6]}{'...' if len(perif_ref) > 6 else ''})")
            R.update(raio=raio, diametro=diam, centro=centro_ref, periferia=perif_ref,
                     ecc_min=raio, ecc_max=diam,
                     ecc_media=float(ecc.mean()))
            R["ecc"] = [round(float(x), 2) for x in ecc]

        # ---- 14 : cobertura ----
        t14 = exec_opcao(14, "cobertura de vertices")
        gul_c = lista_apos("Vertices", t14.split("Heuristica:")[0])
        heu_c = lista_apos("Vertices", t14.split("Heuristica:")[1]) if "Heuristica:" in t14 else []
        gul_r, heu_r = ref_gulosa(n, G), ref_heuristica(n, G)
        conferir(rotulo, "cobertura gulosa: valida", cobertura_valida(G, gul_c), f"({len(gul_c)} vertices)")
        conferir(rotulo, "cobertura heuristica: valida", cobertura_valida(G, heu_c), f"({len(heu_c)} vertices)")
        conferir(rotulo, "cobertura gulosa == replica de referencia", gul_c == gul_r)
        conferir(rotulo, "cobertura heuristica == replica de referencia", heu_c == heu_r)
        conferir(rotulo, "programa informa 'Cobertura valida: sim' nas duas",
                 t14.count("Cobertura valida: sim") == 2)
        R["cobertura_gulosa"] = len(gul_c)
        R["cobertura_heuristica"] = len(heu_c)
        R["cobertura_gulosa_vertices"] = gul_c
        R["cobertura_heuristica_vertices"] = heu_c
        # referencia de qualidade: limite inferior por emparelhamento maximo + otimo exato
        emp = len(nx.max_weight_matching(G, maxcardinality=True))
        R["limite_inf_emparelhamento"] = emp
        limite = 30 if n <= 200 else TEMPO_MILP
        melhor, dual, provada = cobertura_otima(n, G, limite)
        busca = cobertura_busca_aleatoria(n, G) if not provada else None
        candidatos = [x for x in (melhor, len(gul_c), len(heu_c), busca) if x is not None]
        melhor_conhecida = min(candidatos)
        limite_inf = max(emp, math.ceil(dual - 1e-6)) if dual is not None else emp
        if provada:
            limite_inf = melhor_conhecida = melhor
        R["cobertura_otima_provada"] = provada
        R["cobertura_milp_incumbente"] = melhor
        R["cobertura_busca_aleatoria"] = busca
        R["cobertura_melhor_conhecida"] = melhor_conhecida
        R["cobertura_limite_inferior"] = limite_inf
        print(f"  cobertura: guloso={len(gul_c)}  heuristica={len(heu_c)}  "
              f"melhor conhecida={melhor_conhecida} "
              f"({'OTIMO provado' if provada else 'limite inferior ' + str(limite_inf) + ', nao provado'})  "
              f"[MILP={melhor}, busca aleatoria={busca}, emparelhamento={emp}]")

        # ---- tempos (mediana de varias execucoes) ----
        tt = {"agm": [], "metricas": [], "gulosa": [], "heuristica": []}
        for _ in range(REPETICOES):
            if c == 1:
                tt["agm"] += tempos(rodar(arquivo, 12))
                tt["metricas"] += tempos(rodar(arquivo, 13))
            t = tempos(rodar(arquivo, 14))
            tt["gulosa"].append(t[0])
            tt["heuristica"].append(t[1])
        R["tempo_ms_mediana"] = {k: (statistics.median(v) if v else None) for k, v in tt.items()}
        R["tempo_ms_min"] = {k: (min(v) if v else None) for k, v in tt.items()}
        R["tempo_ms_max"] = {k: (max(v) if v else None) for k, v in tt.items()}

        with open(f"saidas/{rotulo}_saida_programa.txt", "w") as f:
            f.write(f"Instancia: {arquivo}  (n={n}, m={m})\n\n" + "\n".join(transcricao))
        resultados[rotulo] = R

    json.dump(resultados, open("resultados.json", "w"), indent=1)
    total, erros = len(checks), [c for c in checks if not c[2]]
    print(f"\n{total - len(erros)}/{total} verificacoes OK")
    for e in erros:
        print("FALHOU:", e)
    sys.exit(1 if erros else 0)


if __name__ == "__main__":
    main()
