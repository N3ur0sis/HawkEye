#!/bin/bash
# Crée le fichier agents.conf s'il n'existe pas
if [ ! -f agents.conf ]; then
    echo "127.0.0.1 9999" > agents.conf
    echo "127.0.0.1 9998" >> agents.conf
    echo "127.0.0.1 9997" >> agents.conf
fi

# Lance les agents en arrière-plan
echo "Lancement de agent-nmap sur le port 9999..."
./bin/agent-nmap 9999 &
PID_NMAP=$!

echo "Lancement de agent-nikto sur le port 9998..."
./bin/agent-nikto 9998 &
PID_NIKTO=$!

echo "Lancement de agent-zap sur le port 9997..."
./bin/agent-zap 9997 &
PID_ZAP=$!


echo "Agents lancées."