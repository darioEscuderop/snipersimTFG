#./run-sniper -d ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/test-2MB-16MB-O3 -g --tfg/dario/tipo_sistema_memoria=1 -g --tfg/dario/memoria=16 --roi -c 1ipc ../scripts_sniper/a.out 


#nohup ./run-sniper -d ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/test-2MB-16MB-O3 -g --tfg/dario/tipo_sistema_memoria=2 -g --tfg/dario/memoria=16 --roi -c 1ipc ../scripts_sniper/a.out > ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/output-2mb-16mb.log 2>&1 &


#nohup ./run-sniper -d ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/test-4KB-16MB-O3 -g --tfg/dario/tipo_sistema_memoria=1 -g --tfg/dario/memoria=16 --roi -c 1ipc ../scripts_sniper/a.out > ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/output-4kb-16mb.log 2>&1 &


#nohup ./run-sniper -d ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/test-4KB-1MB-O3 -g --tfg/dario/tipo_sistema_memoria=1 -g --tfg/dario/memoria=16 --roi -c 1ipc ../scripts_sniper/a.out > ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/output-4kb-1mb.log 2>&1 &


#nohup ./run-sniper -d ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/test-2MB-4MB-O3 -g --tfg/dario/tipo_sistema_memoria=2 -g --tfg/dario/memoria=16 --roi -c 1ipc ../scripts_sniper/a.out > ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/output-2mb-4mb.log 2>&1 &



#./run-sniper  -g --tfg/dario/tipo_sistema_memoria=2 -g --tfg/dario/memoria=1 --roi -c 1ipc ../scripts_sniper/a.out 


#!/bin/bash
# Lista de pruebas
tests=(
    "test-2MB-16MB-O2"
    "test-4KB-16MB-O2"
    "test-4KB-1MB-O2"
    "test-2MB-4MB-O2"
    "test-4KB-1MB-02-TEST-PAGES"
    "test-2MB-16MB-O2-TEST-PAGES"
)

# Lista de tipos de sistema de memoria
mem_types=(
    2
    1
    1
    2
    1
    2
)

# Lista de tamaños de memoria
mem_sizes=(
    16
    16
    16
    16
    16
    16
)

# Lista de nombres de archivos de salida
output_files=(
    "output-2mb-16mb.log"
    "output-4kb-16mb.log"
    "output-4kb-1mb.log"
    "output-2mb-4mb.log"
    "output-4kb-1mb-02-TEST-PAGES.log"
    "output-2mb-16mb-O2-TEST-PAGES.log"
)

# Lista de nombres de archivos .out
out_files=(
    "a.out"
    "a.out"
    "a.out"
    "a.out"
    "test-4kb"
    "test-2mb"
)

# Ejecutar todas las pruebas
for i in ${!tests[@]}; do
    nohup ./run-sniper -d ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/${tests[$i]} -g --tfg/dario/tipo_sistema_memoria=${mem_types[$i]} -g --tfg/dario/memoria=${mem_sizes[$i]} --roi -c 1ipc ../scripts_sniper/${out_files[$i]} > ~/SALIDAS_SNIPER/caracterizacion/tabla_paginas/${output_files[$i]} 2>&1 &
done
