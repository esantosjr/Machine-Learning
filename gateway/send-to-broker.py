# Developed by: E. J. Santos Jr.
# Contact: e.joaojr@gmail.com

import paho.mqtt.client as mqtt
import sys
import base64
import json
import datetime
import time
import os
import subprocess

# parametros
broker = "localhost"
broker_port = 1883
keep_alive_broker = 60
subscribe_topic = "application/+/device/+/rx"

# inicializa MQTT:
client = mqtt.Client()

def init_broker_conection():
    print("[STATUS] Inicializando MQTT...")
    sys.stdout.flush()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(broker, broker_port, keep_alive_broker)
    client.loop_forever()

# Extrai parametros da mensagem recebida pelo packet-forwarder
def get_message_informations(message):
    # {"applicationID":"1","applicationName":"smart-forest","deviceName":"mtldrbc","devEUI":"313135385e376d19",
    # "rxInfo":[{"gatewayID":"b827ebffff2b4da6","name":"test","rssi":-37,"loRaSNR":10,"location":{"latitude":0,"longitude":0,"altitude":0}}],
    # "txInfo":{"frequency":903700000,"dr":3},"adr":true,"fCnt":16,"fPort":1,"data":"SGVsbG8gMQ=="}

    parameters_dict = json.loads(message)
    # parametros podem ser acessados usando: parameters_dict['applicationID'], parameters_dict['devEUI'], ... etc
    return parameters_dict

def schedule_downlink(conf, port, data):
    # mosquitto_pub -h localhost -p 1883 -t application/1/device/313135385e376d19/tx -m "{\"confirmed\": false, \"fPort\": 20, \"data\": \"YmNjNTVj\"}"
    topic = "application/1/device/313135385e376d19/tx"
    client.publish(topic, "{\"confirmed\":" + conf +", \"fPort\":" + port +", \"data\": \"" + data +"\"}")

# Callback - conexao ao broker realizada
def on_connect(client, userdata, flags, rc):
    print("[STATUS] Conectado ao Broker. Resultado de conexao: " + str(rc))
    sys.stdout.flush()

    # topic subscribe
    client.subscribe(subscribe_topic)

# Callback - mensagem recebida do broker
def on_message(client, userdata, msg):
    print("[MSG RECEBIDA] Topico: " + msg.topic)
    sys.stdout.flush()

    received_message = get_message_informations(str(msg.payload))
    # print(json.dumps(received_message, indent=4))

    data = base64.b64decode(received_message['data'])
    data_to_send = "Message=[" + str(received_message['object']['decoded']) + "],RSSI=[" + str(received_message['rxInfo'][0]['rssi']) + "],SNR=[" + str(received_message['rxInfo'][0]['loRaSNR']) + "],Freq=[" + str(received_message['txInfo']['frequency']) + "],SF=[" + str(10-received_message['txInfo']['dr']) + "]\n"
    # print(data_to_send)

    os.system('mosquitto_pub -h test.mosquitto.org -t "data/"' + str(received_message['devEUI']) + ' -m ' + str(msg.payload))
    os.system('mosquitto_pub -h test.mosquitto.org -t "data/"' + str(received_message['devEUI']) + '/filtered -m ' + data_to_send)
    
    # verifica se mensagem eh destinada ao node 802.15.4
    data_splited = str(received_message['object']['decoded']).split(",")
    if(data_splited[0] is "X"):
    	print("Mensagem " + data_splited[1] + " enviada para o node " + data_splited[2])
	    os.system('mosquitto_pub -h 150.162.10.114 -t ' +  data_splited[2] + '/ctrl' + ' -m ' + data_splited[1])

# programa principal:
if __name__ == '__main__':
    init_broker_conection()
