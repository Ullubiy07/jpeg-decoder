## Пример работы
![Демонстрация работы](docs/screenshot.png)

## Сборка
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release  
cmake --build build
```

## Запуск
Открыть изображение можно через **File -> Open** или передать путь аргументом командной строки:
```bash
./build/jpeg-decoder docs/cat.jpg
```
