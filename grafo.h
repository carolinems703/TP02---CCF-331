#ifndef GRAFO_H
#define GRAFO_H

#include <stddef.h>

/* Grafo simples, nao direcionado, com vertices numerados de 1 a n.
   A implementacao da matriz fica escondida no arquivo grafo.c. */
typedef struct grafo Grafo;

typedef struct {
    int v1;
    int v2;
    double peso;
} Aresta;

/* Nos vetores pai e distancia, o vertice v ocupa a posicao v - 1.
   Pai 0 indica raiz; pai -1 indica vertice nao alcancado. */
typedef struct {
    int ordem_grafo;
    int quantidade;
    int *sequencia;
    int *pai;
} BuscaLargura;

typedef struct {
    int ordem_grafo;
    int origem;
    double *distancia;
    int *pai;
} CaminhosMinimos;

typedef struct {
    int ordem_grafo;
    size_t quantidade;
    Aresta *arestas;
    double custo;
} ArvoreGeradora;

/* excentricidade[v - 1] e a maior distancia (ponderada) de v a outro vertice.
   Raio e o menor valor e diametro e o maior valor entre as excentricidades. */
typedef struct {
    int ordem_grafo;
    double *excentricidade;
    double raio;
    double diametro;
} Metricas;

/* vertices guarda os vertices escolhidos, em ordem crescente. */
typedef struct {
    int ordem_grafo;
    int quantidade;
    int *vertices;
} Cobertura;

Grafo *grafo_criar(int ordem);
void grafo_destruir(Grafo *grafo);
/* Retorna 1 em caso de sucesso; 0 para vertices/peso invalidos ou repeticao. */
int grafo_inserir_aresta(Grafo *grafo, int v1, int v2, double peso);
Grafo *grafo_ler_arquivo(const char *nome, char *erro, size_t capacidade_erro);

int grafo_ordem(const Grafo *grafo);
size_t grafo_tamanho(const Grafo *grafo);
/* epsilon(G) = m / n. A normalizada e 2m / (n(n-1)), entre 0 e 1. */
double grafo_densidade(const Grafo *grafo);
double grafo_densidade_normalizada(const Grafo *grafo);
int grafo_vertice_valido(const Grafo *grafo, int vertice);
/* Peso 0 significa ausencia de aresta (ou argumentos invalidos). */
double grafo_peso(const Grafo *grafo, int v1, int v2);
/* vizinhos deve ter espaco para pelo menos n inteiros. Retorna -1 em erro. */
int grafo_vizinhos(const Grafo *grafo, int vertice, int *vizinhos);
int grafo_grau(const Grafo *grafo, int vertice);
/* Retorna 1/0 para sim/nao e -1 em erro. */
int grafo_articulacao(const Grafo *grafo, int vertice);
int grafo_possui_ciclo(const Grafo *grafo);
/* componente deve ter n posicoes. Rotulos comecam em 1. Retorna -1 em erro. */
int grafo_componentes(const Grafo *grafo, int *componente);

BuscaLargura *grafo_busca_largura(const Grafo *grafo, int origem);
void busca_largura_destruir(BuscaLargura *busca);
/* Inclui todas as arestas fora da arvore, mesmo em outras componentes.
   O vetor retornado pertence ao chamador e deve ser liberado com free().
   Sucesso com zero arestas devolve NULL no vetor e quantidade 0. */
int grafo_arestas_fora_arvore(const Grafo *grafo, const BuscaLargura *busca,
                            Aresta **arestas, size_t *quantidade);

CaminhosMinimos *grafo_dijkstra(const Grafo *grafo, int origem);
void caminhos_minimos_destruir(CaminhosMinimos *caminhos);
/* caminho deve ter n posicoes. Retorna quantidade de vertices, 0 se nao
   existe caminho e -1 em erro. Distancia infinita indica inalcançavel. */
int caminhos_minimos_obter(const CaminhosMinimos *caminhos, int destino,
                          int *caminho);

/* Retorna 1/0 para sim/nao e -1 em erro. */
int grafo_conexo(const Grafo *grafo);

/* Circuito Euleriano: todos os graus pares e arestas em uma so componente.
   Grafo sem arestas nao possui circuito. Retorna 1/0 e -1 em erro. */
int grafo_possui_circuito_euleriano(const Grafo *grafo);
/* circuito deve ter m + 1 posicoes. Preenche com a sequencia de vertices
   (o primeiro e igual ao ultimo). Retorna a quantidade de vertices escritos,
   0 se nao existe circuito e -1 em erro. */
int grafo_circuito_euleriano(const Grafo *grafo, int *circuito);

/* Algoritmo de Prim. Retorna NULL se o grafo nao for conexo ou em erro. */
ArvoreGeradora *grafo_arvore_geradora_minima(const Grafo *grafo);
void arvore_geradora_destruir(ArvoreGeradora *arvore);

/* Retorna NULL se o grafo nao for conexo ou em erro. */
Metricas *grafo_metricas(const Grafo *grafo);
void metricas_destruir(Metricas *metricas);

/* Cobertura de vertices: toda aresta tem ao menos uma extremidade escolhida.
   Guloso: escolhe sempre o vertice de maior grau entre as arestas restantes.
   Heuristica: escolhe as duas pontas de arestas descobertas e depois remove
   vertices redundantes. Nenhuma das duas garante o otimo. */
Cobertura *grafo_cobertura_gulosa(const Grafo *grafo);
Cobertura *grafo_cobertura_heuristica(const Grafo *grafo);
/* Retorna 1/0 para sim/nao e -1 em erro. */
int grafo_cobertura_valida(const Grafo *grafo, const Cobertura *cobertura);
void cobertura_destruir(Cobertura *cobertura);

#endif