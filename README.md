# 💓 CardioIA - Fase 3: Monitoramento IoT e Edge Computing

## 📋 Descrição do Projeto

O **CardioIA** é um projeto acadêmico focado na convergência entre tecnologia e saúde cardiovascular. Nesta fase, evoluímos a arquitetura para o monitoramento físico em tempo real. O objetivo é simular um dispositivo vestível (*wearable*) de telemetria hospitalar integrado a um painel de monitoramento inteligente, utilizando arquiteturas modernas de IoT.

Desenvolvemos três camadas principais de solução:
1. **Hardware Simulado (ESP32):** Captura de dados vitais (Temperatura e Batimentos) em tempo real.
2. **Computação de Borda (Edge Computing):** Mecanismo de resiliência e armazenamento offline para evitar perda de dados médicos em caso de queda de rede.
3. **Dashboard Analítico (Fog Computing):** Painel médico em Node-RED com regras de triagem automatizadas para alertas visuais imediatos.

## 👨‍⚕️ Integrantes do Grupo
- <a href="https://www.linkedin.com/in/nicolas--araujo/">Nicolas Antonio Silva Araujo</a> 
- <a href="https://www.linkedin.com/in/vitoria-bagatin-31ba88266/">Vitória Pereira Bagatin</a> 

## 🎬 Vídeo Demonstrativo
Confira a demonstração completa da arquitetura IoT, simulação de queda de rede e funcionamento do Dashboard no Node-RED:
<video src="" autoplay loop muted playsinline width="100%"></video>

---

## 📂 Estrutura de Arquivos

A organização do repositório separa o código do microcontrolador (Borda) das configurações da interface visual (Nuvem/Névoa):

```text
Cardio-IA-IoT/
│
├── Assets/                        # Imagens e PDFs
│
├── node_red_dashboard/
│   └── flows.json                 # Arquivo exportado contendo toda a lógica e visual do painel
│
├── wokwi_hardware/
│   ├── main.cpp                   # Código em C++ do ESP32 (Lógica de sensores, MQTT e Buffer Offline)
│   ├── diagram.json               # Estrutura e pinagem do circuito simulado no Wokwi
│   ├── link_wokwi                 # Link de simulação no wokwi
│   └── wokwi.toml                 # Configurações do ambiente de simulação
│
└── README.md                      # Documentação principal
````

## ⚙️ Arquitetura e Fluxo de Dados (Como os sistemas se conectam)

Para garantir a escalabilidade e a modularidade do sistema em um ambiente hospitalar, o fluxo de dados foi dividido em blocos independentes que se comunicam via protocolo MQTT:

### 🩺 Parte 1: Coleta e Processamento (ESP32 + Sensores)
![ckt](https://github.com/lab-ia-shared/Cardio-IA-3/blob/5c3adfdc2a851b43d028dc7beb53886533119558/Assets/wokwi-ckt.png)
**Ferramenta:** Wokwi Simulator | **Linguagem:** C++
Este módulo atua junto ao corpo do paciente. O código `main.cpp` realiza as seguintes funções:
* **Sensores:** Lê dados de um DHT22 (Temperatura) e um Potenciômetro (simulando a variabilidade dos Batimentos por Minuto - BPM mapeados de 40 a 180).
* **Formatação:** Converte as leituras em um objeto leve `{"temperatura": X, "bpm": Y}`.
* **Transmissão:** Conecta-se à internet e envia o pacote de dados a cada 3 segundos via protocolo MQTT para o *broker* central.

### 📡 Parte 2: O Roteamento de Mensagens (Broker MQTT)
**Ferramenta:** HiveMQ (Servidor Público)
Atua como a "antena" central do hospital. Ele recebe as mensagens publicadas pelo ESP32 no tópico `cardioia/nicolas/sinais` e as distribui instantaneamente para qualquer sistema que esteja escutando este mesmo "canal".

### 🧠 Parte 3: Triagem Inteligente e Dashboard (Node-RED)
![Dashboard1](https://github.com/lab-ia-shared/Cardio-IA-3/blob/5c3adfdc2a851b43d028dc7beb53886533119558/Assets/dashboard-hipotermia-taquicardia.png)
![Dashboard2](https://github.com/lab-ia-shared/Cardio-IA-3/blob/5c3adfdc2a851b43d028dc7beb53886533119558/Assets/dashboard-sinais-normais.png)
![Dashboard3](https://github.com/lab-ia-shared/Cardio-IA-3/blob/5c3adfdc2a851b43d028dc7beb53886533119558/Assets/dashboard-febre-bradicardia.png)

**Ferramenta:** Node-RED (Local/Cloud)
Este módulo consome o arquivo `flows.json`. Ele assina o tópico MQTT e, a cada nova mensagem recebida, aciona um script interno (Nó de Função JavaScript) que atua como médico triador:
* **Diagnóstico da Temperatura:** Classifica em Hipotermia (< 35°C), Normal, Febre ou Febre Alta (> 38.5°C).
* **Diagnóstico Cardíaco:** Classifica em Bradicardia Severa (< 50 BPM), Normal, Taquicardia ou Taquicardia Severa (> 120 BPM).
* **Visualização:** Atualiza gráficos de ponteiro (Gauge) com limites de cores dinâmicos (orientados a dados médicos) e emite alertas de texto como `🚨 CRÍTICO!` em caso de anomalias.

---

## 📊 Arquitetura de Resiliência (Edge Computing em Ação)

Em cenários reais de saúde (UTIs e ambulatórios), o monitoramento não pode falhar se o Wi-Fi do hospital cair. Implementamos uma lógica rigorosa de *Edge Computing* para resolver isso:

1. **Simulação de Queda:** Um Push Button físico no circuito do ESP32 simula a perda instantânea de conectividade Wi-Fi/MQTT.
2. **Buffer Local (Memória Interna):** Ao perder a conexão, o ESP32 para de tentar enviar os dados e passa a salvar as leituras de temperatura e BPM em um *Array* interno contínuo (atuando como o sistema de arquivos SPIFFS/LittleFS).
3. **Sincronização Pós-Queda:** Quando o botão é pressionado novamente (simulando o retorno da rede), o sistema se reconecta e realiza o "despejo" (*flush*) seguro do histórico. Ele envia as mensagens represadas adicionando uma flag `{"historico": true}`, garantindo que o prontuário do paciente fique completo no sistema hospitalar sem sobrecarregar a banda com envios simultâneos.

---

## 🛡️ Governança e Aplicação Médica

Este projeto segue premissas essenciais de confiabilidade para sistemas da saúde (*Healthcare IoT*):

* **Integridade dos Dados:** O uso de MQTT com nível de qualidade (QoS) apropriado e a lógica de persistência na borda garantem que nenhum dado de sinal vital seja perdido ou corrompido durante transmissões instáveis.
* **Dados Simulados vs. Reais:** O uso de potenciômetros na Prova de Conceito (PoC) atende perfeitamente ao requisito de testar o tráfego de dados. Em um cenário real, esses componentes seriam facilmente substituídos por sensores MAX30102 (Oximetria/BPM) sem alterar uma única linha da arquitetura de nuvem.
* **Apoio à Decisão Médica (DSS):** O dashboard construído no Node-RED com regras de cores graduais reduz a carga cognitiva dos enfermeiros. O sistema não toma decisões de medicação, mas prioriza visualmente pacientes críticos, atuando como um poderoso Sistema de Suporte à Decisão.
