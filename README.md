# pthreads

Полное описание алгоритмов и замеров — [docs/report.md](docs/report.md).

## Quick start

```bash
make            # сборка всех бинарников в bin/
make demo       # демонстрация всех трёх заданий -> results/*.csv
```

Через Docker:

```bash
make docker-demo
```

## Задания

| #   | Задание          | Запуск                                            | Результат                |
| --- | ---------------- | ------------------------------------------------- | ------------------------ |
| 1   | Мандельброт      | `./bin/mandelbrot <потоки> <точек>`               | `results/mandelbrot.csv` |
| 2   | N тел            | `./bin/nbody <t_end> <файл_тел>`                  | `results/nbody.csv`      |
| 3   | RW-lock + список | `./bin/llist_<impl> <потоки> <сек> <доля_чтений>` | stdout (ops/s)           |

`<impl>` — `pthread` (системный `pthread_rwlock_t`) или `custom` (собственная реализация).

## Демонстрация по частям

```bash
make demo-mandelbrot   # 4 потока, 10000 точек
make demo-nbody        # 4 потока, t_end=1.0, data/bodies.txt
make demo-rwlock       # список: сравнение pthread rwlock и своей реализации
```

## Бенчмарк

```bash
make bench             # 1/2/4/8 потоков, 5 запусков -> results/bench.csv
```

## Прочее

```bash
make fmt        # clang-format по src/
make fmt-check  # проверка форматирования
make clean      # удалить бинарники и результаты
```

Требуются `gcc` (C11, pthreads) и `make`; для `make fmt` — `clang-format`.
