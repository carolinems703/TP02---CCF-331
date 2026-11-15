# TP02 - CCF 331

Trabalho Prático 2 da disciplina CCF 331.

## Compilação

Para compilar o projeto, execute no terminal:

```bash
make
```

Isso gerará o executável `tp2`.

### Outros comandos do Makefile:
- `make clean`: limpa os arquivos objeto (`.o`) e o executável `tp2`.
- `make test`: executa os testes de validação utilizando o script `validar.py`.

## Execução

Você pode executar o programa passando o caminho do arquivo de grafo como argumento:

```bash
./tp2 01_pequeno_euleriano.txt
```

Ou executar diretamente e informar o arquivo quando solicitado pelo menu:

```bash
./tp2
```

### Arquivos de instância disponíveis para teste:
- `01_pequeno_euleriano.txt`
- `02_estrutural.txt`
- `03_esparso.txt`
- `04_denso.txt`
