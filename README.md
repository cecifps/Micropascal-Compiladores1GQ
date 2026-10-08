# Micro-Pascal — Construção de Compiladores

Implementação da **Primeira Parte do Projeto de Construção de Compiladores** da Universidade Católica de Pernambuco.

O projeto implementa, em **C11**, as duas primeiras etapas de um compilador para a linguagem simplificada **micro-Pascal**:

- analisador léxico (lexer);
- analisador sintático (parser).

A implementação utiliza apenas a biblioteca padrão de C e não utiliza Flex, Bison ou outras bibliotecas externas.

## Estrutura do projeto

```text
.
├── exemplos/
│   ├── paridade.pas
│   ├── soma_impares.pas
│   └── completo.pas
├── src/
│   └── micropascal.c
├── tests/
│   └── test.py
├── .gitignore
├── Makefile
└── README.md
```

### Arquivos

| Arquivo | Função |
|---|---|
| `src/micropascal.c` | Implementação do lexer, parser e programa principal. |
| `exemplos/paridade.pas` | Exemplo de paridade apresentado no enunciado. |
| `exemplos/soma_impares.pas` | Exemplo da soma dos primeiros números ímpares apresentado no enunciado. |
| `exemplos/completo.pas` | Exemplo adicional para testar diferentes recursos da linguagem. |
| `tests/test.py` | Testes automatizados para entradas válidas, inválidas e tokens. |
| `Makefile` | Compilação, execução dos testes e limpeza do executável. |
| `.gitignore` | Ignora arquivos gerados durante a compilação. |

## Requisitos

- GCC ou Clang com suporte a C11;
- Make;
- Python 3 para executar os testes automatizados.

A execução normal do compilador não depende de Python.

## Compilação

Na raiz do projeto:

```bash
make
```

Ou, sem Make:

```bash
cc -std=c11 -Wall -Wextra -Wpedantic -O2 src/micropascal.c -o micropascal
```

## Execução

Para analisar um programa micro-Pascal:

```bash
./micropascal exemplos/paridade.pas
```

Também podem ser executados:

```bash
./micropascal exemplos/soma_impares.pas
./micropascal exemplos/completo.pas
```

Quando o programa é válido, o compilador informa:

```text
Análise léxica e sintática concluída com sucesso.
```

A primeira parte do projeto apenas verifica a estrutura léxica e sintática. Ela não executa o programa e não realiza análise semântica, como verificação de tipos ou de variáveis declaradas.

## Analisador léxico

O lexer reconhece os tokens especificados para o micro-Pascal.

### Identificadores

```text
letra ::= [a-zA-Z_]
digito ::= [0-9]

IDENTIFICADOR ::= letra (letra | digito)*
```

A linguagem diferencia maiúsculas e minúsculas. Portanto:

```text
begin  -> palavra reservada
Begin  -> identificador
```

### Operadores relacionais

```text
<   >   <=   >=   =   <>
```

### Operadores lógico-aritméticos

```text
+   -   *   /
div   and   or   not
```

### Atribuição

```text
:=
```

### Símbolos especiais

```text
)   (   ,   ;   .   :
```

### Palavras reservadas

```text
program  if  then  else  while  do  repeat  until
integer  real  char  begin  end  write  var
div  and  or  not
```

### Literais

Inteiro:

```text
digito+
```

Real:

```text
digito*.digito+
```

Assim, exemplos como estes são reconhecidos:

```text
10
10.25
.5
```

Caractere:

```text
'a'
'0'
'\\n'
'\\t'
```

O lexer também ignora espaço, quebra de linha, tabulação e retorno de carro, conforme o enunciado.

Os exemplos fornecidos no enunciado utilizam comentários `//`; por isso o lexer também ignora comentários desse tipo. Essa extensão é necessária para que os próprios exemplos do enunciado possam ser analisados.

Quando aparece um caractere que não inicia nenhum token válido, é emitida uma mensagem no formato:

```text
Erro léxico no caracter [x]
```

## Analisador sintático

O parser foi implementado por **descida recursiva**.

A gramática abstrata do enunciado possui recursão à esquerda nas expressões. Para implementar o parser, ela foi transformada em níveis de precedência equivalentes, mantendo a associatividade à esquerda indicada no projeto.

A organização das expressões é:

```text
expressao   ::= comparacao { (and | or) comparacao }*
comparacao  ::= soma { (= | <> | < | > | <= | >=) soma }*
soma        ::= produto { (+ | -) produto }*
produto     ::= primaria { (* | / | div) primaria }*
primaria    ::= IDENTIFICADOR
              | INTEIRO_LITERAL
              | REAL_LITERAL
              | CHAR_LITERAL
              | '(' expressao ')'
              | not expressao
```

A precedência segue o enunciado:

1. `*`, `/`, `div`
2. `+`, `-`
3. `=`, `<>`, `<`, `>`, `<=`, `>=`
4. `or`, `and`

Os operadores de cada nível são tratados de forma associativa à esquerda.

O parser reconhece:

- programa e seu identificador;
- seção `var`;
- declarações de variáveis;
- tipos `integer`, `real` e `char`;
- blocos `begin ... end`;
- atribuições;
- `while ... do`;
- `repeat ... until`;
- `if ... then ... else`;
- `write(...)`;
- expressões e operadores.

Em caso de erro sintático, a mensagem segue o formato exigido:

```text
Erro de sintaxe no token [lexema]
```

## Visualização dos tokens

O programa possui também um modo opcional para visualizar o resultado do lexer:

```bash
./micropascal --tokens exemplos/paridade.pas
```

Cada linha apresenta a localização, o tipo do token e seu lexema. O último token apresentado é `EOF`.

Esse modo serve para facilitar a conferência do analisador léxico durante o desenvolvimento e a apresentação.

## Testes

Para compilar e executar todos os testes:

```bash
make test
```

Os testes verificam, entre outros casos:

- os exemplos do projeto;
- identificadores e palavras reservadas;
- sensibilidade a maiúsculas/minúsculas;
- inteiros, reais e caracteres;
- operadores compostos;
- expressões e precedência;
- `if`, `while` e `repeat`;
- comentários `//` presentes nos exemplos;
- caracteres inválidos;
- literais `char` inválidos;
- ausência de `;`;
- tokens depois do ponto final;
- modo de visualização dos tokens.

Para remover o executável gerado:

```bash
make clean
```

## Como colocar no GitHub

Depois de criar um novo repositório vazio no GitHub, na pasta deste projeto:

```bash
git init
git add .
git commit -m "Implementa lexer e parser do micro-Pascal"
git branch -M main
git remote add origin URL_DO_SEU_REPOSITORIO
git push -u origin main
```

Não é necessário colocar o executável `micropascal` no repositório, pois ele é gerado pelo `make` e está no `.gitignore`.
