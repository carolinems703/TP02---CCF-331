#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include "grafo.h"

static int ler_linha(char *texto, int capacidade) {
    size_t tamanho;
    int caractere;
    if (fgets(texto, capacidade, stdin) == NULL)
        return 0;
    tamanho = strlen(texto);
    if (tamanho > 0 && texto[tamanho - 1] == '\n') {
        texto[--tamanho] = '\0';
        if (tamanho > 0 && texto[tamanho - 1] == '\r')
            texto[tamanho - 1] = '\0';
        return 1;
    }
    if (feof(stdin))
        return 1;
    while ((caractere = getchar()) != '\n' && caractere != EOF) {}
    puts("Entrada muito longa. Tente novamente.");
    return -1;
}

/* 1 = sucesso; 0 = fim da entrada; -1 = entrada invalida. */
static int ler_numero(const char *mensagem, int *numero) {
    char texto[128], *fim;
    long valor;
    int estado;
    printf("%s", mensagem);
    fflush(stdout);
    estado = ler_linha(texto, sizeof(texto));
    if (estado != 1)
        return estado;
    errno = 0;
    valor = strtol(texto, &fim, 10);
    if (fim == texto || errno == ERANGE || valor < INT_MIN || valor > INT_MAX) {
        puts("Digite um numero inteiro valido.");
        return -1;
    }
    while (isspace((unsigned char)*fim))
        fim++;
    if (*fim != '\0') {
        puts("Digite apenas um numero inteiro.");
        return -1;
    }
    *numero = (int)valor;
    return 1;
}

static int ler_vertice(const Grafo *grafo, int *vertice) {
    int estado = ler_numero("Informe o vertice: ", vertice);
    if (estado != 1)
        return estado;
    if (!grafo_vertice_valido(grafo, *vertice)) {
        printf("O vertice deve estar entre 1 e %d.\n", grafo_ordem(grafo));
        return -1;
    }
    return 1;
}

static void mostrar_menu(void) {
    puts("\n========================================");
    puts("ANALISE DE REDE SOCIAL");
    puts("========================================");
    puts("1  - Informar ordem do grafo");
    puts("2  - Informar tamanho do grafo");
    puts("3  - Calcular densidade");
    puts("4  - Informar vizinhos de um vertice");
    puts("5  - Informar grau de um vertice");
    puts("6  - Verificar se um vertice e articulacao");
    puts("7  - Executar busca em largura");
    puts("8  - Identificar componentes conexas");
    puts("9  - Verificar existencia de ciclos");
    puts("10 - Calcular caminhos minimos");
    puts("11 - Verificar circuito Euleriano");
    puts("12 - Determinar arvore geradora minima");
    puts("13 - Calcular excentricidade, raio e diametro");
    puts("14 - Calcular cobertura de vertices (guloso e heuristica)");
    puts("0  - Sair");
}

static void mostrar_vizinhos(const Grafo *grafo, int vertice) {
    int *vizinhos = malloc((size_t)grafo_ordem(grafo) * sizeof(int));
    int quantidade, i;
    if (vizinhos == NULL) {
        puts("Memoria insuficiente.");
        return;
    }
    quantidade = grafo_vizinhos(grafo, vertice, vizinhos);
    printf("Vizinhos de %d:", vertice);
    if (quantidade == 0)
        printf(" nenhum (vertice isolado)");
    for (i = 0; i < quantidade; i++)
        printf(" %d", vizinhos[i]);
    putchar('\n');
    free(vizinhos);
}

static void mostrar_componentes(const Grafo *grafo) {
    int n = grafo_ordem(grafo);
    int *rotulos = malloc((size_t)n * sizeof(int));
    int quantidade, c, v;
    if (rotulos == NULL) {
        puts("Memoria insuficiente.");
        return;
    }
    quantidade = grafo_componentes(grafo, rotulos);
    if (quantidade < 0) {
        puts("Nao foi possivel calcular as componentes.");
        free(rotulos);
        return;
    }
    printf("Numero de componentes conexas: %d\n", quantidade);
    for (c = 1; c <= quantidade; c++) {
        printf("Componente %d:", c);
        for (v = 0; v < n; v++)
            if (rotulos[v] == c)
                printf(" %d", v + 1);
        putchar('\n');
    }
    free(rotulos);
}

static void mostrar_busca(const Grafo *grafo, int origem) {
    BuscaLargura *busca = grafo_busca_largura(grafo, origem);
    Aresta *arestas = NULL;
    size_t quantidade = 0, a;
    int i;
    if (busca == NULL) {
        puts("Nao foi possivel executar a busca.");
        return;
    }
    printf("Sequencia de visita a partir de %d:", origem);
    for (i = 0; i < busca->quantidade; i++)
        printf(" %d", busca->sequencia[i]);
    puts("\nArestas da arvore:");
    for (i = 0; i < busca->ordem_grafo; i++)
        if (busca->pai[i] > 0)
            printf("(%d, %d)\n", busca->pai[i], i + 1);
    if (busca->quantidade == 1)
        puts("Nenhuma.");
    if (!grafo_arestas_fora_arvore(grafo, busca, &arestas, &quantidade))
        puts("Nao foi possivel listar as arestas fora da arvore.");
    else {
        printf("Arestas fora da arvore (%zu):\n", quantidade);
        if (quantidade == 0)
            puts("Nenhuma.");
        for (a = 0; a < quantidade; a++) {
            printf("(%d, %d), peso %.2f", arestas[a].v1, arestas[a].v2, arestas[a].peso);
            if (busca->pai[arestas[a].v1 - 1] == -1)
                printf(" [outra componente, nao alcancada]");
            putchar('\n');
        }
    }
    if (busca->quantidade < busca->ordem_grafo) {
        printf("Vertices nao alcancados:");
        for (i = 0; i < busca->ordem_grafo; i++)
            if (busca->pai[i] == -1)
                printf(" %d", i + 1);
        putchar('\n');
    }
    free(arestas);
    busca_largura_destruir(busca);
}

static void mostrar_caminhos(const Grafo *grafo, int origem) {
    CaminhosMinimos *caminhos = grafo_dijkstra(grafo, origem);
    int *caminho = malloc((size_t)grafo_ordem(grafo) * sizeof(int));
    int destino, i, quantidade;
    if (caminhos == NULL || caminho == NULL) {
        puts("Nao foi possivel calcular os caminhos (memoria ou limite numerico).");
        free(caminho);
        caminhos_minimos_destruir(caminhos);
        return;
    }
    for (destino = 1; destino <= grafo_ordem(grafo); destino++) {
        quantidade = caminhos_minimos_obter(caminhos, destino, caminho);
        printf("%d -> %d: ", origem, destino);
        if (quantidade == 0)
            puts("sem caminho (distancia infinita)");
        else if (quantidade < 0)
            puts("erro ao reconstruir caminho");
        else {
            printf("distancia %.2f | caminho: ", caminhos->distancia[destino - 1]);
            for (i = 0; i < quantidade; i++)
                printf("%s%d", i == 0 ? "" : " -> ", caminho[i]);
            putchar('\n');
        }
    }
    free(caminho);
    caminhos_minimos_destruir(caminhos);
}

/* Igualdade de distancias com tolerancia: duas excentricidades iguais na teoria
   (ex.: as duas pontas de um diametro) podem diferir em ~1e-14 por causa da
   ordem das somas em ponto flutuante, entao "==" perderia vertices. */
static int distancias_iguais(double a, double b) {
    return fabs(a - b) <= 1e-9 * (1.0 + fabs(b));
}

static double milissegundos(clock_t inicio) {
    return 1000.0 * (double)(clock() - inicio) / CLOCKS_PER_SEC;
}

static void mostrar_euleriano(const Grafo *grafo) {
    int *circuito;
    int resultado, quantidade, v, i, impares = 0;
    resultado = grafo_possui_circuito_euleriano(grafo);
    if (resultado < 0) {
        puts("Nao foi possivel verificar o circuito Euleriano.");
        return;
    }
    if (resultado == 0) {
        puts("O grafo nao possui circuito Euleriano.");
        if (grafo_tamanho(grafo) == 0) {
            puts("Motivo: o grafo nao possui arestas.");
            return;
        }
        for (v = 1; v <= grafo_ordem(grafo); v++)
            if (grafo_grau(grafo, v) % 2 != 0)
                impares++;
        if (impares == 0) {
            puts("Motivo: todos os graus sao pares, mas as arestas estao em componentes diferentes.");
            return;
        }
        printf("Vertices de grau impar (%d):", impares);
        for (v = 1; v <= grafo_ordem(grafo); v++)
            if (grafo_grau(grafo, v) % 2 != 0)
                printf(" %d", v);
        putchar('\n');
        return;
    }
    circuito = malloc((grafo_tamanho(grafo) + 1) * sizeof(int));
    if (circuito == NULL) {
        puts("Memoria insuficiente.");
        return;
    }
    quantidade = grafo_circuito_euleriano(grafo, circuito);
    if (quantidade < 0) {
        puts("Nao foi possivel construir o circuito.");
        free(circuito);
        return;
    }
    puts("O grafo possui circuito Euleriano.");
    printf("Circuito (%d arestas):\n", quantidade - 1);
    for (i = 0; i < quantidade; i++)
        printf("%s%d", i == 0 ? "" : " -> ", circuito[i]);
    putchar('\n');
    free(circuito);
}

static void mostrar_arvore(const Grafo *grafo) {
    ArvoreGeradora *arvore;
    clock_t inicio;
    double tempo;
    size_t a;
    inicio = clock();
    arvore = grafo_arvore_geradora_minima(grafo);
    tempo = milissegundos(inicio);
    if (arvore == NULL) {
        puts("Nao foi possivel determinar a arvore (o grafo precisa ser conexo).");
        return;
    }
    printf("Arvore geradora minima (%zu arestas):\n", arvore->quantidade);
    for (a = 0; a < arvore->quantidade; a++)
        printf("(%d, %d), peso %.2f\n", arvore->arestas[a].v1,
               arvore->arestas[a].v2, arvore->arestas[a].peso);
    printf("Custo total: %.2f\n", arvore->custo);
    printf("Tempo: %.3f ms\n", tempo);
    arvore_geradora_destruir(arvore);
}

static void mostrar_metricas(const Grafo *grafo) {
    Metricas *metricas;
    clock_t inicio;
    double tempo;
    int v;
    inicio = clock();
    metricas = grafo_metricas(grafo);
    tempo = milissegundos(inicio);
    if (metricas == NULL) {
        puts("Nao foi possivel calcular as metricas (o grafo precisa ser conexo).");
        return;
    }
    puts("Excentricidade de cada vertice:");
    for (v = 0; v < metricas->ordem_grafo; v++)
        printf("%d: %.2f\n", v + 1, metricas->excentricidade[v]);
    printf("Raio: %.2f\n", metricas->raio);
    printf("Diametro: %.2f\n", metricas->diametro);
    printf("Centro (excentricidade = raio):");
    for (v = 0; v < metricas->ordem_grafo; v++)
        if (distancias_iguais(metricas->excentricidade[v], metricas->raio))
            printf(" %d", v + 1);
    printf("\nPeriferia (excentricidade = diametro):");
    for (v = 0; v < metricas->ordem_grafo; v++)
        if (distancias_iguais(metricas->excentricidade[v], metricas->diametro))
            printf(" %d", v + 1);
    putchar('\n');
    printf("Tempo: %.3f ms\n", tempo);
    metricas_destruir(metricas);
}

static void mostrar_cobertura(const Grafo *grafo, const char *nome,
                              Cobertura *(*algoritmo)(const Grafo *)) {
    Cobertura *cobertura;
    clock_t inicio;
    double tempo;
    int i;
    inicio = clock();
    cobertura = algoritmo(grafo);
    tempo = milissegundos(inicio);
    if (cobertura == NULL) {
        printf("Nao foi possivel calcular a cobertura (%s).\n", nome);
        return;
    }
    printf("%s: %d vertices\n", nome, cobertura->quantidade);
    printf("Vertices:");
    for (i = 0; i < cobertura->quantidade; i++)
        printf(" %d", cobertura->vertices[i]);
    printf("\nCobertura valida: %s\n",
           grafo_cobertura_valida(grafo, cobertura) == 1 ? "sim" : "nao");
    printf("Tempo: %.3f ms\n", tempo);
    cobertura_destruir(cobertura);
}

int main(int argc, char **argv) {
    Grafo *grafo;
    char nome[1024], erro[256];
    const char *arquivo;
    int opcao, vertice, estado, resultado;
    if (argc > 2) {
        fprintf(stderr, "Uso: %s [arquivo_do_grafo]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2)
        arquivo = argv[1];
    else {
        printf("Informe o arquivo do grafo: ");
        fflush(stdout);
        if (ler_linha(nome, sizeof(nome)) != 1)
            return EXIT_FAILURE;
        arquivo = nome;
    }
    grafo = grafo_ler_arquivo(arquivo, erro, sizeof(erro));
    if (grafo == NULL) {
        fprintf(stderr, "Erro: %s\n", erro);
        return EXIT_FAILURE;
    }
    printf("Grafo carregado: %d vertices e %zu arestas.\n",
           grafo_ordem(grafo), grafo_tamanho(grafo));
    for (;;) {
        mostrar_menu();
        estado = ler_numero("Digite a opcao: ", &opcao);
        if (estado == 0 || (estado == 1 && opcao == 0))
            break;
        if (estado < 0)
            continue;
        if (opcao == 4 || opcao == 5 || opcao == 6 || opcao == 7 || opcao == 10) {
            estado = ler_vertice(grafo, &vertice);
            if (estado == 0)
                break;
            if (estado < 0)
                continue;
        }
        switch (opcao) {
            case 1: printf("Ordem: %d vertices.\n", grafo_ordem(grafo)); break;
            case 2: printf("Tamanho: %zu arestas.\n", grafo_tamanho(grafo)); break;
            case 3:
                printf("Densidade epsilon(G) = m/n: %.6f.\n", grafo_densidade(grafo));
                printf("Densidade normalizada = 2m/[n(n-1)]: %.6f (%.2f%%).\n",
                       grafo_densidade_normalizada(grafo),
                       100.0 * grafo_densidade_normalizada(grafo));
                break;
            case 4: mostrar_vizinhos(grafo, vertice); break;
            case 5: printf("Grau de %d: %d.\n", vertice, grafo_grau(grafo, vertice)); break;
            case 6:
                resultado = grafo_articulacao(grafo, vertice);
                if (resultado < 0)
                    puts("Nao foi possivel testar articulacao.");
                else
                    printf("O vertice %d %s articulacao.\n", vertice,
                           resultado ? "e" : "nao e");
                break;
            case 7: mostrar_busca(grafo, vertice); break;
            case 8: mostrar_componentes(grafo); break;
            case 9:
                resultado = grafo_possui_ciclo(grafo);
                puts(resultado < 0 ? "Nao foi possivel verificar ciclos." :
                     resultado ? "O grafo possui ciclo." : "O grafo nao possui ciclo.");
                break;
            case 10: mostrar_caminhos(grafo, vertice); break;
            case 11: mostrar_euleriano(grafo); break;
            case 12: mostrar_arvore(grafo); break;
            case 13: mostrar_metricas(grafo); break;
            case 14:
                mostrar_cobertura(grafo, "Guloso", grafo_cobertura_gulosa);
                putchar('\n');
                mostrar_cobertura(grafo, "Heuristica", grafo_cobertura_heuristica);
                break;
            default: puts("Opcao invalida.");
        }
    }
    grafo_destruir(grafo);
    puts("Programa encerrado.");
    return EXIT_SUCCESS;
}