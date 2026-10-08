program Completo;
var
  i, n : integer;
  x : real;
  c : char;
begin
  n := 10;
  i := 0;
  x := .5;
  c := 'a';
  while i < n and not (i = 7) do
    begin
      if i <> 3 then
        write(c);
      else
        write('\t');
      i := i + 1;
    end;
  repeat
    begin
      i := i - 1;
    end;
  until i = 0;
end.

