#!/bin/bash
# Сборка руководства пользователя в PDF: docs/manual/manual.pdf
# Нужен TeX Live с xelatex, polyglossia, tcolorbox; шрифты Liberation и DejaVu.
set -e
cd "$(dirname "$0")"
for pass in 1 2; do  # второй проход - оглавление и ссылки
    xelatex -interaction=nonstopmode -halt-on-error manual.tex > /dev/null
done
rm -f manual.aux manual.log manual.out manual.toc
echo "=== Готово: $(pwd)/manual.pdf"
