#!/bin/bash

# Script de prueba para sistema Pub/Sub UDP
# Uso: ./test_udp.sh <num_publishers> <num_subscribers> <mensajes_por_publisher>

if [ $# -ne 3 ]; then
    echo "Uso: $0 <num_publishers> <num_subscribers> <mensajes_por_publisher>"
    echo "Ejemplo: $0 3 5 10"
    exit 1
fi

NUM_PUBLISHERS=$1
NUM_SUBSCRIBERS=$2
MSGS_PER_PUBLISHER=$3

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Arrays para almacenar PIDs
PUBLISHER_PIDS=()
SUBSCRIBER_PIDS=()

# Directorio para logs
LOG_DIR="./udp_logs"
mkdir -p $LOG_DIR

echo -e "${GREEN}=== Sistema de Prueba UDP Pub/Sub ===${NC}"
echo -e "${BLUE}Publishers: $NUM_PUBLISHERS${NC}"
echo -e "${BLUE}Subscribers: $NUM_SUBSCRIBERS${NC}"
echo -e "${BLUE}Mensajes por publisher: $MSGS_PER_PUBLISHER${NC}"
echo ""

# Verificar que publisher_udp esté compilado
if [ ! -f "./publisher_udp" ]; then
    echo -e "${RED}Error: publisher_udp no encontrado${NC}"
    echo -e "${YELLOW}Compile con: gcc publisher_udp.c -o publisher_udp${NC}"
    exit 1
fi

# Array de partidos de ejemplo
PARTIDOS=("A_vs_B" "C_vs_D" "E_vs_F" "G_vs_H" "I_vs_J")

# Iniciar Subscribers
echo -e "${GREEN}Iniciando $NUM_SUBSCRIBERS subscribers...${NC}"
for ((i=1; i<=NUM_SUBSCRIBERS; i++)); do
    PARTIDO_IDX=$((i % ${#PARTIDOS[@]}))
    PARTIDO=${PARTIDOS[$PARTIDO_IDX]}
    
    LOG_FILE="$LOG_DIR/subscriber_${i}_${PARTIDO}.log"
    
    # Crear subscriber que se suscribe udpmáticamente
    (
        echo "$PARTIDO" | ./subscriber_udp > "$LOG_FILE" 2>&1
    ) &
    
    SUBSCRIBER_PIDS+=($!)
    echo -e "  Subscriber $i (PID: ${SUBSCRIBER_PIDS[$i-1]}) suscrito a ${YELLOW}$PARTIDO${NC}"
    sleep 0.1
done

echo ""
echo -e "${BLUE}Esperando 2 segundos para que subscribers se registren...${NC}"
sleep 2

# Iniciar Publishers
echo ""
echo -e "${GREEN}Iniciando $NUM_PUBLISHERS publishers...${NC}"
for ((i=1; i<=NUM_PUBLISHERS; i++)); do
    PARTIDO_IDX=$((i % ${#PARTIDOS[@]}))
    PARTIDO=${PARTIDOS[$PARTIDO_IDX]}
    
    LOG_FILE="$LOG_DIR/publisher_${i}_${PARTIDO}.log"
    
    # Ejecutar publisher_udp directamente
    ./publisher_udp "$PARTIDO" "$i" "$MSGS_PER_PUBLISHER" > "$LOG_FILE" 2>&1 &
    
    PUBLISHER_PIDS+=($!)
    echo -e "  Publisher $i (PID: ${PUBLISHER_PIDS[$i-1]}) enviando a ${YELLOW}$PARTIDO${NC}"
    sleep 0.2
done

echo ""
echo -e "${GREEN}=== Todos los procesos iniciados ===${NC}"
echo ""

# Esperar a que se envíen todos los mensajes
TOTAL_MSGS=$((NUM_PUBLISHERS * MSGS_PER_PUBLISHER))
WAIT_TIME=$((MSGS_PER_PUBLISHER / 5 + 3))
echo -e "${BLUE}Esperando envío de $TOTAL_MSGS mensajes (~${WAIT_TIME}s)...${NC}"
sleep $WAIT_TIME

# Mostrar resumen
echo ""
echo -e "${GREEN}=== Resumen de Procesos ===${NC}"
echo ""
echo -e "${YELLOW}PUBLISHERS:${NC}"
for ((i=0; i<NUM_PUBLISHERS; i++)); do
    PARTIDO_IDX=$(((i+1) % ${#PARTIDOS[@]}))
    PARTIDO=${PARTIDOS[$PARTIDO_IDX]}
    if ps -p ${PUBLISHER_PIDS[$i]} > /dev/null 2>&1; then
        STATUS="${GREEN}ACTIVO${NC}"
    else
        STATUS="${BLUE}TERMINADO${NC}"
    fi
    echo -e "  PID ${PUBLISHER_PIDS[$i]} - Publisher $((i+1)) - Partido: $PARTIDO - Estado: $STATUS"
done

echo ""
echo -e "${YELLOW}SUBSCRIBERS:${NC}"
for ((i=0; i<NUM_SUBSCRIBERS; i++)); do
    PARTIDO_IDX=$(((i+1) % ${#PARTIDOS[@]}))
    PARTIDO=${PARTIDOS[$PARTIDO_IDX]}
    if ps -p ${SUBSCRIBER_PIDS[$i]} > /dev/null 2>&1; then
        STATUS="${GREEN}ACTIVO${NC}"
    else
        STATUS="${RED}TERMINADO${NC}"
    fi
    echo -e "  PID ${SUBSCRIBER_PIDS[$i]} - Subscriber $((i+1)) - Partido: $PARTIDO - Estado: $STATUS"
done

echo ""
echo -e "${GREEN}=== Análisis de Mensajes Recibidos ===${NC}"
echo ""

# Analizar logs de subscribers para detectar pérdidas
for ((i=1; i<=NUM_SUBSCRIBERS; i++)); do
    PARTIDO_IDX=$((i % ${#PARTIDOS[@]}))
    PARTIDO=${PARTIDOS[$PARTIDO_IDX]}
    LOG_FILE="$LOG_DIR/subscriber_${i}_${PARTIDO}.log"
    
    if [ -f "$LOG_FILE" ]; then
        MSG_COUNT=$(grep -c "\[EVENTO\]" "$LOG_FILE" 2>/dev/null || echo "0")
        
        # Contar publishers que deberían enviar a este partido
        EXPECTED_PUBLISHERS=0
        for ((p=1; p<=NUM_PUBLISHERS; p++)); do
            P_PARTIDO_IDX=$((p % ${#PARTIDOS[@]}))
            P_PARTIDO=${PARTIDOS[$P_PARTIDO_IDX]}
            if [ "$P_PARTIDO" == "$PARTIDO" ]; then
                EXPECTED_PUBLISHERS=$((EXPECTED_PUBLISHERS + 1))
            fi
        done
        EXPECTED_MSGS=$((EXPECTED_PUBLISHERS * MSGS_PER_PUBLISHER))
        
        echo -e "${BLUE}Subscriber $i ($PARTIDO):${NC}"
        echo -e "  Mensajes recibidos: $MSG_COUNT"
        echo -e "  Mensajes esperados: $EXPECTED_MSGS (de $EXPECTED_PUBLISHERS publishers)"
        
        if [ $MSG_COUNT -eq $EXPECTED_MSGS ]; then
            echo -e "  ${GREEN}✓ Todos los mensajes recibidos correctamente${NC}"
        elif [ $MSG_COUNT -gt $EXPECTED_MSGS ]; then
            DUPLICADOS=$((MSG_COUNT - EXPECTED_MSGS))
            echo -e "  ${RED}⚠ Mensajes duplicados! $DUPLICADOS extras${NC}"
        else
            PERDIDOS=$((EXPECTED_MSGS - MSG_COUNT))
            PORCENTAJE=$((MSG_COUNT * 100 / EXPECTED_MSGS))
            echo -e "  ${YELLOW}⚠ Mensajes perdidos! $PERDIDOS faltantes (${PORCENTAJE}% recibido)${NC}"
        fi
        
        # Extraer y mostrar números de mensaje únicos por publisher
        echo -e "  Desglose por publisher:"
        for ((p=1; p<=NUM_PUBLISHERS; p++)); do
            P_PARTIDO_IDX=$((p % ${#PARTIDOS[@]}))
            P_PARTIDO=${PARTIDOS[$P_PARTIDO_IDX]}
            if [ "$P_PARTIDO" == "$PARTIDO" ]; then
                MSGS_FROM_PUB=$(grep "\[PUB-$p\]" "$LOG_FILE" 2>/dev/null | wc -l)
                UNIQUE_MSGS=$(grep "\[PUB-$p\]" "$LOG_FILE" 2>/dev/null | sed 's/.*MSG#\([0-9]*\).*/\1/' | sort -n -u | tr '\n' ',' | sed 's/,$//')
                if [ ! -z "$UNIQUE_MSGS" ]; then
                    echo -e "    PUB-$p: $MSGS_FROM_PUB msgs - Números: [$UNIQUE_MSGS]"
                fi
            fi
        done
        echo ""
    fi
done

echo ""
echo -e "${GREEN}=== Análisis de Mensajes Enviados ===${NC}"
echo ""
for ((i=1; i<=NUM_PUBLISHERS; i++)); do
    PARTIDO_IDX=$((i % ${#PARTIDOS[@]}))
    PARTIDO=${PARTIDOS[$PARTIDO_IDX]}
    LOG_FILE="$LOG_DIR/publisher_${i}_${PARTIDO}.log"
    
    if [ -f "$LOG_FILE" ]; then
        SENT_COUNT=$(grep -c "Enviado:" "$LOG_FILE" 2>/dev/null || echo "0")
        echo -e "${BLUE}Publisher $i ($PARTIDO):${NC} Envió $SENT_COUNT mensajes"
    fi
done

echo ""
echo -e "${GREEN}=== Estado Final ===${NC}"
echo -e "${YELLOW}Logs guardados en: $LOG_DIR/${NC}"
echo ""

# Obtener PIDs activos
ACTIVE_SUBS=()

for pid in "${SUBSCRIBER_PIDS[@]}"; do
    if ps -p $pid > /dev/null 2>&1; then
        ACTIVE_SUBS+=($pid)
    fi
done

if [ ${#ACTIVE_SUBS[@]} -gt 0 ]; then
    echo -e "Subscribers siguen activos. Para detenerlos:"
    echo -e "  ${RED}kill ${ACTIVE_SUBS[@]}${NC}"
else
    echo -e "${GREEN}Todos los procesos han terminado.${NC}"
fi

echo ""
echo -e "Para ver logs de subscribers en tiempo real:"
echo -e "  tail -f $LOG_DIR/subscriber_*.log"
echo ""
echo -e "Para ver logs de publishers:"
echo -e "  cat $LOG_DIR/publisher_*.log"
echo ""

echo -e "${GREEN}Script completado.${NC}"