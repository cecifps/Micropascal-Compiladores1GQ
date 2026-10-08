"""Testes de caixa-preta do analisador Micro-Pascal (Python 3, sem dependências)."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "micropascal"
count = 0


def check(source, valid=True, error=None, tokens=False):
    global count
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "teste.pas"
        path.write_bytes(source.encode("utf-8"))
        command = [str(BIN)]
        if tokens:
            command.append("--tokens")
        command.append(str(path))
        result = subprocess.run(command, text=True, capture_output=True)

    assert (result.returncode == 0) == valid, (
        source, result.returncode, result.stdout, result.stderr
    )
    if error:
        assert error in result.stderr, (error, result.stderr)
    count += 1
    return result.stdout


# Exemplos versionados no repositório.
for path in sorted((ROOT / "exemplo").glob("*.pas")):
    check(path.read_text(encoding="utf-8"))

# Programas válidos e construções principais.
check("program p; var begin end.")
check("program p; var A,_b2:char; begin A:='a'; _b2:='\\n'; end.")
check("program p; var begin if 1 then if 2 then write(1); else write(2); end.")
check("program p; var begin write(1 < 2 <= 3 = 4 <> 5 >= 6 > 7); end.")
check("program p; var begin write(1 + 2 * 3 div 4 / 5 - 6 and 7 or not 8); end.")
check("program p;\r\nvar\tbegin\r\nend. // comentário final")

# Erros sintáticos e lexemas reportados.
for source, lexeme in [
    ("program p; var begin", "EOF"),
    ("program p; var begin write(1) end.", "end"),
    ("program p; var begin end", "EOF"),
    ("program p; var begin end. x", "x"),
    ("program p; var begin ; end.", ";"),
    ("Program p; var begin end.", "Program"),
    ("program p; var begin write(-1); end.", "-"),
    ("program p; var begin write(1.); end.", "."),
    ("program p; var begin write(); end.", ")"),
    ("program p; var begin repeat write(1); write(2); until 1; end.", "write"),
    ("program p; var begin begin end end.", "end"),
]:
    check(source, False, f"Erro de sintaxe no token [{lexeme}]")

# Erros léxicos. O modo --tokens permite testar o lexer isoladamente.
for source, character in [
    ("@", "@"),
    ("'ab'", "b"),
    (r"'\r'", "\\"),
    ("' '", " "),
    ("'a", "EOF"),
    ("_\x00", r"\0"),
    ("'_'", "_"),
]:
    check(source, False, f"Erro léxico no caracter [{character}]", tokens=True)

# Formato e tipos básicos de tokens.
output = check(r"begin Begin _x2 123 .5 12.75 12. <= >= <> := '\n' '\t'", tokens=True)
rows = [row.split("\t", 2) for row in output.splitlines()]
assert [row[1] for row in rows] == [
    "begin", "IDENTIFICADOR", "IDENTIFICADOR", "INTEIRO_LITERAL",
    "REAL_LITERAL", "REAL_LITERAL", "INTEIRO_LITERAL", ".", "<=", ">=",
    "<>", ":=", "CHAR_LITERAL", "CHAR_LITERAL", "EOF"
]
assert rows[0][0] == "1:1" and rows[1][2] == "Begin"

print(f"{count} testes passaram.")
