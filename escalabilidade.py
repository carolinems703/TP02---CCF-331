#!/usr/bin/env python3
"""
Experimento complementar de escalabilidade (nao sao as instancias oficiais).

 (a) densidade fixa em 20%, n variando de 200 a 1000;
 (b) n fixo em 500, densidade variando de 2% a 50%.

Para cada grafo gerado (conexo, pesos distintos), mede a mediana de 5
execucoes dos tempos que o proprio programa imprime (AGM, metricas,
cobertura gulosa e heuristica) e o tamanho das coberturas.
Gera: escalabilidade.json
"""
import json
import os
import re
import statistics
import subprocess
import tempfile

from gerar_instancias import gerar_denso, escrever

REP = 5


def tempos(opcao, arquivo):
    p = subprocess.run(["./tp2", arquivo], input=f"{opcao}\n0\n", capture_output=True, text=True)
    return [float(x) for x in re.findall(r"Tempo: ([\d.]+) ms", p.stdout)], p.stdout


def medir(n, dens, semente):
    n, pesos = gerar_denso(n=n, densidade=dens, semente=semente)
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
        nome = f.name
    escrever(nome, n, pesos)
    m = len(pesos)
    r = {"n": n, "m": m, "densidade_normalizada": 2 * m / (n * (n - 1))}
    for chave, op, idx in (("agm_ms", 12, 0), ("metricas_ms", 13, 0), ("gulosa_ms", 14, 0), ("heuristica_ms", 14, 1)):
        r[chave] = statistics.median(tempos(op, nome)[0][idx] for _ in range(REP))
    out = tempos(14, nome)[1]
    r["cobertura_gulosa"], r["cobertura_heuristica"] = [int(x) for x in re.findall(r"(?:Guloso|Heuristica): (\d+) vertices", out)]
    os.remove(nome)
    return r


if __name__ == "__main__":
    res = {"por_n": [], "por_densidade": []}
    for n in (200, 400, 600, 800, 1000):
        res["por_n"].append(medir(n, 0.20, 100 + n))
        print(res["por_n"][-1])
    for d in (0.02, 0.05, 0.10, 0.20, 0.40, 0.50):
        res["por_densidade"].append(medir(500, d, 900 + int(d * 100)))
        print(res["por_densidade"][-1])
    json.dump(res, open("escalabilidade.json", "w"), indent=1)
