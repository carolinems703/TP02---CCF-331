#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include "grafo.h"

/* Fila auxiliar privada: usada apenas pelos algoritmos deste arquivo. */
typedef struct fila Fila;

struct fila {
    int *dados;
    int capacidade;
    int inicio;
    int fim;
    int quantidade;
};

static Fila *fila_criar(int capacidade) {
    Fila *fila;
    if (capacidade <= 0 || (size_t)capacidade > SIZE_MAX / sizeof(int))
        return NULL;
    fila = malloc(sizeof(Fila));
    if (fila == NULL)
        return NULL;
    fila->dados = malloc((size_t)capacidade * sizeof(int));
    if (fila->dados == NULL) {
        free(fila);
        return NULL;
    }
    fila->capacidade = capacidade;
    fila->inicio = 0;
    fila->fim = 0;
    fila->quantidade = 0;
    return fila;
}

static void fila_destruir(Fila *fila) {
    if (fila != NULL) {
        free(fila->dados);
        free(fila);
    }
}

static int fila_vazia(const Fila *fila) {
    return fila == NULL || fila->quantidade == 0;
}

static int fila_inserir(Fila *fila, int valor) {
    if (fila == NULL || fila->quantidade == fila->capacidade)
        return 0;
    fila->dados[fila->fim] = valor;
    fila->fim = (fila->fim + 1) % fila->capacidade;
    fila->quantidade++;
    return 1;
}

static int fila_remover(Fila *fila, int *valor) {
    if (fila_vazia(fila) || valor == NULL)
        return 0;
    *valor = fila->dados[fila->inicio];
    fila->inicio = (fila->inicio + 1) % fila->capacidade;
    fila->quantidade--;
    return 1;
}


struct grafo {
    int ordem;
    size_t tamanho;
    double *matriz;
};

/* u e v sao indices internos: de 0 a n - 1. */
#define PESO(g, u, v) ((g)->matriz[(size_t)(u) * (g)->ordem + (v)])

Grafo *grafo_criar(int ordem) {
    Grafo *grafo;
    size_t n;
    if (ordem <= 0)
        return NULL;
    n = (size_t)ordem;
    if (n > SIZE_MAX / n || n * n > SIZE_MAX / sizeof(double))
        return NULL;
    grafo = malloc(sizeof(Grafo));
    if (grafo == NULL)
        return NULL;
    grafo->matriz = calloc(n * n, sizeof(double));
    if (grafo->matriz == NULL) {
        free(grafo);
        return NULL;
    }
    grafo->ordem = ordem;
    grafo->tamanho = 0;
    return grafo;
}

void grafo_destruir(Grafo *grafo) {
    if (grafo != NULL) {
        free(grafo->matriz);
        free(grafo);
    }
}

int grafo_vertice_valido(const Grafo *grafo, int vertice) {
    return grafo != NULL && vertice >= 1 && vertice <= grafo->ordem;
}

int grafo_inserir_aresta(Grafo *grafo, int v1, int v2, double peso) {
    if (!grafo_vertice_valido(grafo, v1) || !grafo_vertice_valido(grafo, v2)
        || v1 == v2 || !isfinite(peso) || peso <= 0.0)
        return 0;
    v1--;
    v2--;
    if (PESO(grafo, v1, v2) != 0.0)
        return 0;
    PESO(grafo, v1, v2) = peso;
    PESO(grafo, v2, v1) = peso;
    grafo->tamanho++;
    return 1;
}

static void informar_erro(char *erro, size_t capacidade, const char *mensagem,
                          int linha) {
    if (erro != NULL && capacidade > 0) {
        if (linha > 0)
            snprintf(erro, capacidade, "Linha %d: %s", linha, mensagem);
        else
            snprintf(erro, capacidade, "%s", mensagem);
    }
}

Grafo *grafo_ler_arquivo(const char *nome, char *erro, size_t capacidade_erro) {
    FILE *arquivo;
    Grafo *grafo = NULL;
    int ordem, v1, v2;
    double peso;
    int linha_atual = 1;

    if (erro != NULL && capacidade_erro > 0)
        erro[0] = '\0';

    if (nome == NULL) {
        informar_erro(erro, capacidade_erro, "Nome de arquivo invalido.", 0);
        return NULL;
    }

    arquivo = fopen(nome, "r");
    if (arquivo == NULL) {
        informar_erro(erro, capacidade_erro, "Nao foi possivel abrir o arquivo.", 0);
        return NULL;
    }

    /* Lê a primeira linha: ordem do grafo */
    if (fscanf(arquivo, "%d", &ordem) != 1 || ordem <= 0) {
        informar_erro(erro, capacidade_erro, "Erro ao ler a ordem do grafo na primeira linha.", linha_atual);
        fclose(arquivo);
        return NULL;
    }

    grafo = grafo_criar(ordem);
    if (grafo == NULL) {
        informar_erro(erro, capacidade_erro, "Memoria insuficiente para criar o grafo.", 0);
        fclose(arquivo);
        return NULL;
    }

    /* Lê o restante do arquivo: v1 v2 peso */
    linha_atual++;
    while (fscanf(arquivo, "%d %d %lf", &v1, &v2, &peso) == 3) {
        if (!grafo_vertice_valido(grafo, v1) || !grafo_vertice_valido(grafo, v2)) {
            informar_erro(erro, capacidade_erro, "Vertice fora do intervalo de 1 a n.", linha_atual);
            grafo_destruir(grafo);
            fclose(arquivo);
            return NULL;
        }
        if (v1 == v2) {
            informar_erro(erro, capacidade_erro, "Lacos nao sao permitidos neste grafo simples.", linha_atual);
            grafo_destruir(grafo);
            fclose(arquivo);
            return NULL;
        }
        if (!grafo_inserir_aresta(grafo, v1, v2, peso)) {
            informar_erro(erro, capacidade_erro, "Aresta repetida ou peso invalido.", linha_atual);
            grafo_destruir(grafo);
            fclose(arquivo);
            return NULL;
        }
        linha_atual++;
    }

    fclose(arquivo);
    return grafo;
}

int grafo_ordem(const Grafo *grafo) {
    return grafo == NULL ? 0 : grafo->ordem;
}

size_t grafo_tamanho(const Grafo *grafo) {
    return grafo == NULL ? 0 : grafo->tamanho;
}

double grafo_densidade(const Grafo *grafo) {
    if (grafo == NULL)
        return 0.0;
    return (double)grafo->tamanho / grafo->ordem;
}

double grafo_densidade_normalizada(const Grafo *grafo) {
    if (grafo == NULL || grafo->ordem < 2)
        return 0.0;
    return (2.0 * grafo->tamanho) / ((double)grafo->ordem * (grafo->ordem - 1));
}

double grafo_peso(const Grafo *grafo, int v1, int v2) {
    if (!grafo_vertice_valido(grafo, v1) || !grafo_vertice_valido(grafo, v2))
        return 0.0;
    return PESO(grafo, v1 - 1, v2 - 1);
}

int grafo_vizinhos(const Grafo *grafo, int vertice, int *vizinhos) {
    int v, quantidade = 0;
    if (!grafo_vertice_valido(grafo, vertice) || vizinhos == NULL)
        return -1;
    for (v = 0; v < grafo->ordem; v++)
        if (PESO(grafo, vertice - 1, v) > 0.0)
            vizinhos[quantidade++] = v + 1;
    return quantidade;
}

int grafo_grau(const Grafo *grafo, int vertice) {
    int v, grau = 0;
    if (!grafo_vertice_valido(grafo, vertice))
        return -1;
    for (v = 0; v < grafo->ordem; v++)
        if (PESO(grafo, vertice - 1, v) > 0.0)
            grau++;
    return grau;
}

/* 
   Algoritmo de Roy (Fecho Transitivo de Warshall) para componentes conexas.
   Permite ignorar um vertice para testar articulacoes. 
*/
static int componentes_roy_ignorando(const Grafo *grafo, int ignorado, int *rotulos) {
    int i, j, k, n, quantidade = 0;
    int *T; /* Matriz de fecho transitivo simulada em vetor */

    if (grafo == NULL || rotulos == NULL)
        return -1;

    n = grafo->ordem;
    T = calloc((size_t)n * n, sizeof(int));
    if (T == NULL)
        return -1;

    /* Passo 1: Inicializa a matriz de alcance direto */
    for (i = 0; i < n; i++) {
        if (i == ignorado) continue;
        T[i * n + i] = 1; /* Um vertice sempre alcanca a si mesmo */
        for (j = 0; j < n; j++) {
            if (j == ignorado) continue;
            if (PESO(grafo, i, j) > 0.0) {
                T[i * n + j] = 1;
            }
        }
    }

    /* Passo 2: Algoritmo de Roy-Warshall */
    for (k = 0; k < n; k++) {
        if (k == ignorado) continue;
        for (i = 0; i < n; i++) {
            if (i == ignorado) continue;
            /* Otimizacao: so processa o laco mais interno se 'i' alcanca 'k' */
            if (T[i * n + k]) { 
                for (j = 0; j < n; j++) {
                    if (j == ignorado) continue;
                    /* Se 'i' alcanca 'k', e 'k' alcanca 'j', entao 'i' alcanca 'j' */
                    if (T[k * n + j]) {
                        T[i * n + j] = 1;
                    }
                }
            }
        }
    }

    /* Passo 3: Atribuicao de rotulos das componentes com base no fecho transitivo */
    for (i = 0; i < n; i++)
        rotulos[i] = 0;

    for (i = 0; i < n; i++) {
        if (i == ignorado || rotulos[i] != 0)
            continue;
        
        quantidade++;
        /* Todos os vertices alcancaveis por 'i' recebem o id desta componente */
        for (j = 0; j < n; j++) {
            if (j != ignorado && T[i * n + j]) {
                rotulos[j] = quantidade;
            }
        }
    }

    free(T);
    return quantidade;
}

int grafo_componentes(const Grafo *grafo, int *componente) {
    return componentes_roy_ignorando(grafo, -1, componente);
}

int grafo_articulacao(const Grafo *grafo, int vertice) {
    int *rotulos;
    int antes, depois;
    if (!grafo_vertice_valido(grafo, vertice))
        return -1;
    rotulos = malloc((size_t)grafo->ordem * sizeof(int));
    if (rotulos == NULL)
        return -1;
        
    /* Chama o algoritmo de Roy */
    antes = componentes_roy_ignorando(grafo, -1, rotulos);
    depois = componentes_roy_ignorando(grafo, vertice - 1, rotulos);
    
    free(rotulos);
    if (antes < 0 || depois < 0)
        return -1;
    return depois > antes;
}

int grafo_possui_ciclo(const Grafo *grafo) {
    int *rotulos;
    int componentes;
    if (grafo == NULL)
        return -1;
    rotulos = malloc((size_t)grafo->ordem * sizeof(int));
    if (rotulos == NULL)
        return -1;
    componentes = grafo_componentes(grafo, rotulos);
    free(rotulos);
    if (componentes < 0)
        return -1;
    /* Uma floresta com n vertices e c componentes tem n - c arestas.
       Toda aresta excedente fecha um ciclo no grafo simples nao dirigido. */
    return grafo->tamanho > (size_t)(grafo->ordem - componentes);
}

BuscaLargura *grafo_busca_largura(const Grafo *grafo, int origem) {
    BuscaLargura *busca;
    Fila *fila;
    int u, v;
    if (!grafo_vertice_valido(grafo, origem))
        return NULL;
    busca = calloc(1, sizeof(BuscaLargura));
    if (busca == NULL)
        return NULL;
    busca->ordem_grafo = grafo->ordem;
    busca->sequencia = malloc((size_t)grafo->ordem * sizeof(int));
    busca->pai = malloc((size_t)grafo->ordem * sizeof(int));
    fila = fila_criar(grafo->ordem);
    if (busca->sequencia == NULL || busca->pai == NULL || fila == NULL) {
        busca_largura_destruir(busca);
        fila_destruir(fila);
        return NULL;
    }
    for (v = 0; v < grafo->ordem; v++)
        busca->pai[v] = -1;
    busca->pai[origem - 1] = 0;
    fila_inserir(fila, origem - 1);
    while (fila_remover(fila, &u)) {
        busca->sequencia[busca->quantidade++] = u + 1;
        for (v = 0; v < grafo->ordem; v++) {
            if (PESO(grafo, u, v) > 0.0 && busca->pai[v] == -1) {
                busca->pai[v] = u + 1;
                fila_inserir(fila, v);
            }
        }
    }
    fila_destruir(fila);
    return busca;
}

void busca_largura_destruir(BuscaLargura *busca) {
    if (busca != NULL) {
        free(busca->sequencia);
        free(busca->pai);
        free(busca);
    }
}

int grafo_arestas_fora_arvore(const Grafo *grafo, const BuscaLargura *busca,
                            Aresta **arestas, size_t *quantidade) {
    int u, v;
    size_t total = 0, indice = 0;
    if (grafo == NULL || busca == NULL || arestas == NULL || quantidade == NULL
        || busca->ordem_grafo != grafo->ordem)
        return 0;
    *arestas = NULL;
    *quantidade = 0;
    /* Percorremos so a metade superior, evitando contar a mesma aresta duas vezes. */
    for (u = 0; u < grafo->ordem; u++)
        for (v = u + 1; v < grafo->ordem; v++)
            if (PESO(grafo, u, v) > 0.0 && busca->pai[u] != v + 1
                && busca->pai[v] != u + 1)
                total++;
    if (total == 0)
        return 1;
    if (total > SIZE_MAX / sizeof(Aresta))
        return 0;
    *arestas = malloc(total * sizeof(Aresta));
    if (*arestas == NULL)
        return 0;
    for (u = 0; u < grafo->ordem; u++) {
        for (v = u + 1; v < grafo->ordem; v++) {
            if (PESO(grafo, u, v) > 0.0 && busca->pai[u] != v + 1
                && busca->pai[v] != u + 1) {
                (*arestas)[indice].v1 = u + 1;
                (*arestas)[indice].v2 = v + 1;
                (*arestas)[indice].peso = PESO(grafo, u, v);
                indice++;
            }
        }
    }
    *quantidade = total;
    return 1;
}

CaminhosMinimos *grafo_dijkstra(const Grafo *grafo, int origem) {
    CaminhosMinimos *caminhos;
    int *fechado;
    int i, u, v;
    if (!grafo_vertice_valido(grafo, origem))
        return NULL;
    caminhos = calloc(1, sizeof(CaminhosMinimos));
    if (caminhos == NULL)
        return NULL;
    caminhos->ordem_grafo = grafo->ordem;
    caminhos->origem = origem;
    caminhos->distancia = malloc((size_t)grafo->ordem * sizeof(double));
    caminhos->pai = malloc((size_t)grafo->ordem * sizeof(int));
    fechado = calloc((size_t)grafo->ordem, sizeof(int));
    if (caminhos->distancia == NULL || caminhos->pai == NULL || fechado == NULL) {
        caminhos_minimos_destruir(caminhos);
        free(fechado);
        return NULL;
    }
    for (v = 0; v < grafo->ordem; v++) {
        caminhos->distancia[v] = INFINITY;
        caminhos->pai[v] = -1;
    }
    caminhos->distancia[origem - 1] = 0.0;
    caminhos->pai[origem - 1] = 0;
    for (i = 0; i < grafo->ordem; i++) {
        double menor = INFINITY;
        u = -1;
        /* Busca linear pelo vertice aberto mais proximo. */
        for (v = 0; v < grafo->ordem; v++) {
            if (!fechado[v] && caminhos->distancia[v] < menor) {
                menor = caminhos->distancia[v];
                u = v;
            }
        }
        if (u == -1)
            break; /* Os vertices restantes sao inalcançaveis. */
        fechado[u] = 1;
        for (v = 0; v < grafo->ordem; v++) {
            double peso = PESO(grafo, u, v);
            if (!fechado[v] && peso > 0.0) {
                double alternativa = caminhos->distancia[u] + peso;
                /* Se a soma exceder a faixa de double, evitamos devolver
                   um vertice alcancavel como se fosse desconexo. */
                if (!isfinite(alternativa)) {
                    free(fechado);
                    caminhos_minimos_destruir(caminhos);
                    return NULL;
                }
                if (alternativa < caminhos->distancia[v]) {
                    caminhos->distancia[v] = alternativa;
                    caminhos->pai[v] = u + 1;
                }
            }
        }
    }
    free(fechado);
    return caminhos;
}

void caminhos_minimos_destruir(CaminhosMinimos *caminhos) {
    if (caminhos != NULL) {
        free(caminhos->distancia);
        free(caminhos->pai);
        free(caminhos);
    }
}

int caminhos_minimos_obter(const CaminhosMinimos *caminhos, int destino,
                          int *caminho) {
    int atual, quantidade = 0, i;
    if (caminhos == NULL || caminho == NULL || destino < 1
        || destino > caminhos->ordem_grafo)
        return -1;
    if (!isfinite(caminhos->distancia[destino - 1]))
        return 0;
    atual = destino;
    while (atual != 0) {
        if (atual < 1 || atual > caminhos->ordem_grafo
            || quantidade >= caminhos->ordem_grafo)
            return -1;
        caminho[quantidade++] = atual;
        atual = caminhos->pai[atual - 1];
    }
    /* A sequencia foi montada do destino para a origem: precisamos inverter. */
    for (i = 0; i < quantidade / 2; i++) {
        int auxiliar = caminho[i];
        caminho[i] = caminho[quantidade - 1 - i];
        caminho[quantidade - 1 - i] = auxiliar;
    }
    return quantidade;
}

int grafo_conexo(const Grafo *grafo) {
    BuscaLargura *busca;
    int conexo;
    if (grafo == NULL)
        return -1;
    busca = grafo_busca_largura(grafo, 1);
    if (busca == NULL)
        return -1;
    conexo = busca->quantidade == grafo->ordem;
    busca_largura_destruir(busca);
    return conexo;
}

int grafo_possui_circuito_euleriano(const Grafo *grafo) {
    BuscaLargura *busca;
    int v, grau, inicio = -1, resultado = 1;
    if (grafo == NULL)
        return -1;
    if (grafo->tamanho == 0)
        return 0;
    for (v = 0; v < grafo->ordem; v++) {
        grau = grafo_grau(grafo, v + 1);
        if (grau % 2 != 0)
            return 0;
        if (grau > 0 && inicio == -1)
            inicio = v + 1;
    }
    /* Vertices isolados nao atrapalham: so as arestas precisam estar
       na mesma componente. */
    busca = grafo_busca_largura(grafo, inicio);
    if (busca == NULL)
        return -1;
    for (v = 0; v < grafo->ordem; v++)
        if (grafo_grau(grafo, v + 1) > 0 && busca->pai[v] == -1)
            resultado = 0;
    busca_largura_destruir(busca);
    return resultado;
}

/* Algoritmo de Hierholzer: anda por arestas ainda nao usadas ate ficar preso
   (sempre no vertice de partida) e so entao grava o vertice no circuito. */
int grafo_circuito_euleriano(const Grafo *grafo, int *circuito) {
    unsigned char *usada;
    int *pilha, *proximo;
    int n, u, v, inicio = 0, topo = 0, quantidade = 0, resultado;
    if (grafo == NULL || circuito == NULL)
        return -1;
    resultado = grafo_possui_circuito_euleriano(grafo);
    if (resultado != 1)
        return resultado;
    n = grafo->ordem;
    usada = calloc((size_t)n * n, sizeof(unsigned char));
    pilha = malloc((grafo->tamanho + 1) * sizeof(int));
    proximo = calloc((size_t)n, sizeof(int));
    if (usada == NULL || pilha == NULL || proximo == NULL) {
        free(usada);
        free(pilha);
        free(proximo);
        return -1;
    }
    while (grafo_grau(grafo, inicio + 1) == 0)
        inicio++;
    pilha[topo++] = inicio;
    while (topo > 0) {
        u = pilha[topo - 1];
        /* Procura a proxima aresta de u que ainda nao foi percorrida. */
        while (proximo[u] < n && (PESO(grafo, u, proximo[u]) <= 0.0
               || usada[(size_t)u * n + proximo[u]]))
            proximo[u]++;
        if (proximo[u] < n) {
            v = proximo[u];
            usada[(size_t)u * n + v] = 1;
            usada[(size_t)v * n + u] = 1;
            pilha[topo++] = v;
        } else {
            circuito[quantidade++] = u + 1;
            topo--;
        }
    }
    free(usada);
    free(pilha);
    free(proximo);
    return quantidade;
}

void arvore_geradora_destruir(ArvoreGeradora *arvore) {
    if (arvore != NULL) {
        free(arvore->arestas);
        free(arvore);
    }
}

ArvoreGeradora *grafo_arvore_geradora_minima(const Grafo *grafo) {
    ArvoreGeradora *arvore;
    double *chave;
    int *pai, *dentro;
    int i, u, v;
    if (grafo == NULL || grafo_conexo(grafo) != 1)
        return NULL;
    arvore = calloc(1, sizeof(ArvoreGeradora));
    if (arvore == NULL)
        return NULL;
    arvore->ordem_grafo = grafo->ordem;
    arvore->arestas = malloc((size_t)grafo->ordem * sizeof(Aresta));
    chave = malloc((size_t)grafo->ordem * sizeof(double));
    pai = malloc((size_t)grafo->ordem * sizeof(int));
    dentro = calloc((size_t)grafo->ordem, sizeof(int));
    if (arvore->arestas == NULL || chave == NULL || pai == NULL || dentro == NULL) {
        arvore_geradora_destruir(arvore);
        free(chave);
        free(pai);
        free(dentro);
        return NULL;
    }
    for (v = 0; v < grafo->ordem; v++) {
        chave[v] = INFINITY;
        pai[v] = -1;
    }
    chave[0] = 0.0;
    pai[0] = 0;
    for (i = 0; i < grafo->ordem; i++) {
        double menor = INFINITY;
        u = -1;
        /* Busca linear pelo vertice fora da arvore mais barato de ligar.
           Em caso de empate, fica o de menor numero. */
        for (v = 0; v < grafo->ordem; v++) {
            if (!dentro[v] && chave[v] < menor) {
                menor = chave[v];
                u = v;
            }
        }
        if (u == -1)
            break;
        dentro[u] = 1;
        if (pai[u] > 0) {
            arvore->arestas[arvore->quantidade].v1 = pai[u];
            arvore->arestas[arvore->quantidade].v2 = u + 1;
            arvore->arestas[arvore->quantidade].peso = PESO(grafo, pai[u] - 1, u);
            arvore->quantidade++;
            arvore->custo += PESO(grafo, pai[u] - 1, u);
        }
        for (v = 0; v < grafo->ordem; v++) {
            double peso = PESO(grafo, u, v);
            if (!dentro[v] && peso > 0.0 && peso < chave[v]) {
                chave[v] = peso;
                pai[v] = u + 1;
            }
        }
    }
    free(chave);
    free(pai);
    free(dentro);
    return arvore;
}

void metricas_destruir(Metricas *metricas) {
    if (metricas != NULL) {
        free(metricas->excentricidade);
        free(metricas);
    }
}

Metricas *grafo_metricas(const Grafo *grafo) {
    Metricas *metricas;
    CaminhosMinimos *caminhos;
    int u, v;
    if (grafo == NULL || grafo_conexo(grafo) != 1)
        return NULL;
    metricas = calloc(1, sizeof(Metricas));
    if (metricas == NULL)
        return NULL;
    metricas->ordem_grafo = grafo->ordem;
    metricas->excentricidade = malloc((size_t)grafo->ordem * sizeof(double));
    if (metricas->excentricidade == NULL) {
        metricas_destruir(metricas);
        return NULL;
    }
    metricas->raio = INFINITY;
    metricas->diametro = 0.0;
    for (u = 0; u < grafo->ordem; u++) {
        caminhos = grafo_dijkstra(grafo, u + 1);
        if (caminhos == NULL) {
            metricas_destruir(metricas);
            return NULL;
        }
        metricas->excentricidade[u] = 0.0;
        for (v = 0; v < grafo->ordem; v++)
            if (caminhos->distancia[v] > metricas->excentricidade[u])
                metricas->excentricidade[u] = caminhos->distancia[v];
        caminhos_minimos_destruir(caminhos);
        if (metricas->excentricidade[u] < metricas->raio)
            metricas->raio = metricas->excentricidade[u];
        if (metricas->excentricidade[u] > metricas->diametro)
            metricas->diametro = metricas->excentricidade[u];
    }
    return metricas;
}

void cobertura_destruir(Cobertura *cobertura) {
    if (cobertura != NULL) {
        free(cobertura->vertices);
        free(cobertura);
    }
}

/* Monta a cobertura a partir do vetor escolhido (posicao v = vertice v + 1). */
static Cobertura *cobertura_montar(int ordem, const int *escolhido) {
    Cobertura *cobertura = calloc(1, sizeof(Cobertura));
    int v;
    if (cobertura == NULL)
        return NULL;
    cobertura->ordem_grafo = ordem;
    cobertura->vertices = malloc((size_t)ordem * sizeof(int));
    if (cobertura->vertices == NULL) {
        free(cobertura);
        return NULL;
    }
    for (v = 0; v < ordem; v++)
        if (escolhido[v])
            cobertura->vertices[cobertura->quantidade++] = v + 1;
    return cobertura;
}

Cobertura *grafo_cobertura_gulosa(const Grafo *grafo) {
    Cobertura *cobertura;
    int *grau, *escolhido;
    int v, melhor;
    if (grafo == NULL)
        return NULL;
    grau = malloc((size_t)grafo->ordem * sizeof(int));
    escolhido = calloc((size_t)grafo->ordem, sizeof(int));
    if (grau == NULL || escolhido == NULL) {
        free(grau);
        free(escolhido);
        return NULL;
    }
    /* grau[v] conta so as arestas de v que ainda nao foram cobertas. */
    for (v = 0; v < grafo->ordem; v++)
        grau[v] = grafo_grau(grafo, v + 1);
    for (;;) {
        melhor = -1;
        for (v = 0; v < grafo->ordem; v++)
            if (!escolhido[v] && grau[v] > 0
                && (melhor == -1 || grau[v] > grau[melhor]))
                melhor = v;
        if (melhor == -1)
            break;
        escolhido[melhor] = 1;
        for (v = 0; v < grafo->ordem; v++)
            if (PESO(grafo, melhor, v) > 0.0 && !escolhido[v])
                grau[v]--;
        grau[melhor] = 0;
    }
    cobertura = cobertura_montar(grafo->ordem, escolhido);
    free(grau);
    free(escolhido);
    return cobertura;
}

Cobertura *grafo_cobertura_heuristica(const Grafo *grafo) {
    Cobertura *cobertura;
    int *escolhido;
    int u, v, redundante;
    if (grafo == NULL)
        return NULL;
    escolhido = calloc((size_t)grafo->ordem, sizeof(int));
    if (escolhido == NULL)
        return NULL;
    /* Fase 1: toda aresta ainda descoberta leva as suas duas extremidades. */
    for (u = 0; u < grafo->ordem; u++)
        for (v = u + 1; v < grafo->ordem; v++)
            if (PESO(grafo, u, v) > 0.0 && !escolhido[u] && !escolhido[v]) {
                escolhido[u] = 1;
                escolhido[v] = 1;
            }
    /* Fase 2: um vertice cujos vizinhos estao todos escolhidos pode sair. */
    for (u = 0; u < grafo->ordem; u++) {
        if (!escolhido[u])
            continue;
        redundante = 1;
        for (v = 0; v < grafo->ordem && redundante; v++)
            if (PESO(grafo, u, v) > 0.0 && !escolhido[v])
                redundante = 0;
        if (redundante)
            escolhido[u] = 0;
    }
    cobertura = cobertura_montar(grafo->ordem, escolhido);
    free(escolhido);
    return cobertura;
}

int grafo_cobertura_valida(const Grafo *grafo, const Cobertura *cobertura) {
    int *escolhido;
    int i, u, v, valida = 1;
    if (grafo == NULL || cobertura == NULL
        || cobertura->ordem_grafo != grafo->ordem)
        return -1;
    escolhido = calloc((size_t)grafo->ordem, sizeof(int));
    if (escolhido == NULL)
        return -1;
    for (i = 0; i < cobertura->quantidade; i++) {
        if (!grafo_vertice_valido(grafo, cobertura->vertices[i])) {
            free(escolhido);
            return -1;
        }
        escolhido[cobertura->vertices[i] - 1] = 1;
    }
    for (u = 0; u < grafo->ordem && valida; u++)
        for (v = u + 1; v < grafo->ordem && valida; v++)
            if (PESO(grafo, u, v) > 0.0 && !escolhido[u] && !escolhido[v])
                valida = 0;
    free(escolhido);
    return valida;
}