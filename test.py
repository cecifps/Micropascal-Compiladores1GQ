import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "micropascal"
EX = ROOT / "exemplos"
TMP = ROOT / "tests" / "tmp.pas"


def run_text(text, *args):
    TMP.write_text(text, encoding="utf-8")
    try:
        return subprocess.run([str(BIN), *args, str(TMP)], text=True, capture_output=True)
    finally:
        TMP.unlink(missing_ok=True)


def run(path, *args):
    return subprocess.run([str(BIN), *args, str(path)], text=True, capture_output=True)


def assert_ok(path):
    r = run(path)
    assert r.returncode == 0, f"{path}:\n{r.stdout}\n{r.stderr}"


def assert_error(text):
    r = run_text(text)
    assert r.returncode != 0, f"Entrada deveria ser inválida:\n{text}"


# Exemplos fornecidos no enunciado + exemplo complementar.
assert_ok(EX / "paridade.pas")
assert_ok(EX / "soma_impares.pas")
assert_ok(EX / "completo.pas")

# Todos os grupos principais de tokens aparecem em uma entrada válida.
tokens = """program Tokens;
var
  abc_1 : integer;
  r : real;
  c : char;
begin
  abc_1 := 10;
  r := .5 + 10.25 - 2 * 3 / 4 div 2;
  c := '\\n';
  if abc_1 < 20 then
    write(c);
  else
    write('a');
  while abc_1 >= 1 and abc_1 <= 20 do
    abc_1 := abc_1 - 1;
  repeat
    abc_1 := abc_1 + 1;
  until abc_1 <> 10;
  if not (abc_1 = 10) or abc_1 > 0 then
    write('0');
end.
"""
assert_ok(EX / "completo.pas")
r = run_text(tokens)
assert r.returncode == 0, f"Entrada de tokens/comandos falhou:\n{r.stderr}"

# Sensibilidade a maiúsculas/minúsculas: Begin é identificador, não palavra reservada.
assert_error("program X; var x: integer; Begin x := 1; end.")

# Caractere inválido.
assert_error("program X; var x: integer; begin x := 1 @ 2; end.")

# Atribuição sem ponto e vírgula.
assert_error("program X; var x: integer; begin x := 1 end.")

# Token depois do ponto final.
assert_error("program X; var x: integer; begin x := 1; end. x")

# Literal de caractere inválido.
assert_error("program X; var c: char; begin c := 'ab'; end.")

# Escape não permitido.
assert_error("program X; var c: char; begin c := '\\r'; end.")

# Verificação do modo de tokens e dos tokens compostos.
r = run(EX / "paridade.pas", "--tokens")
assert r.returncode == 0, r.stderr
assert "program" in r.stdout
assert ":=" in r.stdout
assert "div" in r.stdout
assert "EOF" in r.stdout

print("Todos os testes passaram.")
