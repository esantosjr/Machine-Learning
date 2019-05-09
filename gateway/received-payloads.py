# Developed by: E. J. Santos Jr.
# Contact: e.joaojr@gmail.com

import paho.mqtt.client as mqtt
import sys
import base64
import json
import datetime

# parametros
broker = "localhost"
broker_port = 1883
keep_alive_broker = 60
subscribe_topic = "gateway/+/rx"

# inicializa MQTT:
client = mqtt.Client()

now = datetime.datetime.now()
file_name = "/home/pi/udp_" + str(now.year) + "_" + str(now.month) + "_" + str(now.day) + ".txt"

def init_broker_conection():
    print("[STATUS] Inicializando MQTT...")
    sys.stdout.flush()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(broker, broker_port, keep_alive_broker)
    client.loop_forever()

# Extrai parametros da mensagem recebida pelo packet-forwarder
def get_message_informations(message):
    # gateway/b827ebffff2b4da6/rx {"rxInfo":{"mac":"b827ebffff2b4da6","timestamp":1226228956,"frequency":902500000,"channel":1,"rfChain":0,"crcStatus":1,
    # "codeRate":"4/5","rssi":-16,"loRaSNR":10,"size":24,"dataRate":{"modulation":"LORA","spreadFactor":10,"bandwidth":125},"board":0,"antenna":0},
    # "phyPayload":"QGOHdgAAAQABNlckmnE637XJPXf1cVkC"}

    parameters_dict = json.loads(message)
    # parametros podem ser acessados usando: parameters_dict['applicationID'], parameters_dict['devEUI'], ... etc
    return parameters_dict

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

    # data = base64.b64decode(received_message['phyPayload'])
    # print(data)

    data_to_save = str(received_message['rxInfo']['rssi']) + "," + str(received_message['rxInfo']['loRaSNR']) + "," + str(received_message['rxInfo']['frequency']) + ",SF" + str(received_message['rxInfo']['dataRate']['spreadFactor']) + "\n"
    print(data_to_save)
    sys.stdout.flush()

    file = open(file_name, "a")
    file.write(data_to_save)
    file.close()

# programa principal:
if __name__ == '__main__':
    init_broker_conection()
