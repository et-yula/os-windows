# C++ Monolith App Template

## Getting Started

1. Do `python tester.py`.

## Run linter

1. Do `pip install cpplint`.

2. Do `find . -name *.hpp -o -name *.cpp -type f | xargs cpplint`.

## Run formater

https://zed0.co.uk/clang-format-configurator/

1. Do `sudo apt-get install clang-format`.

2. Do `find . -name '*.cpp' -o -name '*.hpp' | xargs clang-format --style=google --dry-run --Werror`.

## Thanks

- <https://gitlab.com/Lipovsky/twist>

- <https://github.com/vityaman-edu/bst>
