#!/usr/bin/env bash
set -e

# Directorio de discos de prueba
TESTDISKS_DIR="samples/testdisks"
# Ejecutable de referencia
SOLUTION_EXEC="samples/diskimageaccess_soln_x86"

echo "Verificando existencia de ejecutables..."
if [ ! -f "./diskimageaccess" ]; then
  echo "ERROR: No se encuentra el ejecutable diskimageaccess"
  exit 1
fi

if [ ! -f "$SOLUTION_EXEC" ]; then
  echo "ADVERTENCIA: No se encuentra el ejecutable de referencia en $SOLUTION_EXEC"
  echo "Continuando sin comparar con el ejecutable de referencia"
  NO_REFERENCE=1
else
  echo "Ejecutable de referencia encontrado"
  chmod +x "$SOLUTION_EXEC" || echo "ADVERTENCIA: No se pudieron cambiar permisos del ejecutable de referencia"
  NO_REFERENCE=0
fi

echo "Iniciando pruebas básicas con archivos .gold..."
echo "----------------------------------------"

# Probar combinación -ip con los tres discos
for disk in basicDiskImage depthFileDiskImage dirFnameSizeDiskImage; do
  echo "Probando -ip en $disk..."
  ./diskimageaccess -ip $TESTDISKS_DIR/$disk > out_${disk}.txt
  
  # Verificar que el archivo .gold existe
  if [ ! -f "$TESTDISKS_DIR/${disk}.gold" ]; then
    echo "ERROR: No se encuentra el archivo $TESTDISKS_DIR/${disk}.gold"
    continue
  fi
  
  # Comparar con archivo .gold
  if diff -q out_${disk}.txt $TESTDISKS_DIR/${disk}.gold > /dev/null; then
    echo "✓ Coincide con archivo .gold"
  else
    echo "✗ No coincide con archivo .gold"
    echo "Diferencias:"
    diff out_${disk}.txt $TESTDISKS_DIR/${disk}.gold | head -10
    exit 1
  fi
done

echo "----------------------------------------"
echo "Todas las pruebas básicas pasaron correctamente!"

if [ $NO_REFERENCE -eq 0 ]; then
  echo ""
  echo "Iniciando pruebas adicionales con opciones -i y -p..."
  echo "----------------------------------------"
  
  # Probar las opciones -i y -p por separado
  for opt in "i" "p"; do
    for disk in basicDiskImage depthFileDiskImage dirFnameSizeDiskImage; do
      echo "Probando -$opt en $disk..."
      
      # Ejecutar nuestro programa
      ./diskimageaccess -$opt $TESTDISKS_DIR/$disk > our_output_${opt}_${disk}.txt
      
      # Ejecutar el programa de referencia
      $SOLUTION_EXEC -$opt $TESTDISKS_DIR/$disk > ref_output_${opt}_${disk}.txt
      
      # Comparar con la salida del ejecutable de referencia
      if diff -q our_output_${opt}_${disk}.txt ref_output_${opt}_${disk}.txt > /dev/null; then
        echo "✓ Coincide con salida del ejecutable de referencia"
      else
        echo "✗ No coincide con salida del ejecutable de referencia"
        echo "Diferencias:"
        diff our_output_${opt}_${disk}.txt ref_output_${opt}_${disk}.txt | head -10
        exit 1
      fi
    done
  done
  
  echo "----------------------------------------"
  echo "Todas las pruebas adicionales pasaron correctamente!"
fi

# Limpiar archivos temporales
echo "Limpiando archivos temporales..."
rm -f out_*.txt our_output_*.txt ref_output_*.txt

echo "----------------------------------------"
echo "¡TODAS LAS PRUEBAS PASARON CORRECTAMENTE!" 