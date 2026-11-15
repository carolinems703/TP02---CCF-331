#!/usr/bin/env python3
"""
Gerador das instancias 3 e 4 do Trabalho Pratico 2 (CCF-331).

Formato de saida (o mesmo do TP1):
    linha 1 : n  (ordem do grafo)
    demais  : v1 v2 peso   (uma aresta por linha, vertices de 1 a n, peso real > 0)

Uso:
    python3 gerar_instancias.py            # gera 03_esparso.txt e 04_denso.txt

As sementes estao fixas, entao rodar o script de novo reproduz exatamente os
mesmos arquivos.
"""
import random

SEMENTE_3 = 2026_03
SEMENTE_4 = 2026_04


def escrever(nome, n, pesos):
    """pesos: dict {(u, v): peso} com u < v."""
    with open(nome, "w") as f:
        f.write(f"{n}\n")
        for (u, v) in sorted(pesos):
            f.write(f"{u} {v} {pesos[(u, v)]:.2f}\n")


def pesos_distintos(rng, arestas, minimo_centavos, maximo_centavos):
    """Sorteia um peso real (2 casas) para cada aresta, sem repetir valores.
    Pesos distintos => a AGM do grafo e unica, o que facilita a validacao."""
    valores = rng.sample(range(minimo_centavos, maximo_centavos), len(arestas))
    return {a: valores[i] / 100.0 for i, a in enumerate(sorted(arestas))}


def chave(u, v):
    return (u, v) if u < v else (v, u)


def gerar_esparso(n=120, ciclos_extras=30, hubs=6, semente=SEMENTE_3):
    """Instancia 3: grafo esparso, conexo e COM circuito Euleriano.

    Construcao:
      1. um ciclo hamiltoniano sobre uma permutacao aleatoria dos vertices
         (garante conexao e grau 2 em todos);
      2. 'ciclos_extras' ciclos curtos (3 a 7 vertices). Cada ciclo soma
         exatamente 2 ao grau de seus vertices, entao todos os graus
         continuam PARES -> existe circuito Euleriano;
      3. metade dos ciclos extras passa por um de poucos 'hubs', criando
         vertices de grau bem maior que a media (graus heterogeneos).
    """
    rng = random.Random(semente)
    perm = list(range(1, n + 1))
    rng.shuffle(perm)
    arestas = set()
    for i in range(n):
        arestas.add(chave(perm[i], perm[(i + 1) % n]))

    centros = rng.sample(range(1, n + 1), hubs)
    feitos = 0
    while feitos < ciclos_extras:
        tamanho = rng.randint(3, 7)
        if rng.random() < 0.5:
            hub = rng.choice(centros)
            resto = rng.sample([v for v in range(1, n + 1) if v != hub], tamanho - 1)
            vertices = [hub] + resto
        else:
            vertices = rng.sample(range(1, n + 1), tamanho)
        novas = [chave(vertices[i], vertices[(i + 1) % tamanho]) for i in range(tamanho)]
        if any(a in arestas for a in novas):
            continue  # grafo simples: sem arestas repetidas
        arestas.update(novas)
        feitos += 1

    return n, pesos_distintos(rng, arestas, 100, 5000)  # pesos de 1.00 a 49.99


def gerar_denso(n=500, densidade=0.20, semente=SEMENTE_4):
    """Instancia 4: grafo maior e mais denso, conexo (arvore aleatoria + arestas
    extras sorteadas ate atingir a densidade normalizada pedida)."""
    rng = random.Random(semente)
    perm = list(range(1, n + 1))
    rng.shuffle(perm)
    arestas = set()
    for i in range(1, n):
        arestas.add(chave(perm[i], perm[rng.randrange(i)]))  # arvore geradora aleatoria

    alvo = round(densidade * n * (n - 1) / 2)
    restantes = [(u, v) for u in range(1, n + 1) for v in range(u + 1, n + 1)
                 if (u, v) not in arestas]
    arestas.update(rng.sample(restantes, alvo - len(arestas)))

    return n, pesos_distintos(rng, arestas, 100, 100000)  # pesos de 1.00 a 999.99


if __name__ == "__main__":
    n3, p3 = gerar_esparso()
    escrever("03_esparso.txt", n3, p3)
    print(f"03_esparso.txt: n={n3}, m={len(p3)}")

    n4, p4 = gerar_denso()
    escrever("04_denso.txt", n4, p4)
    print(f"04_denso.txt:   n={n4}, m={len(p4)}")
