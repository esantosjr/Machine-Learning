## Instruções para geração e configuração da imagem do sistema operacional do *gateway*

#### Geraçăo da imagem

Para criar a imagem do sistema operacional para a *Raspberry Pi* já com os módulos do *Lora Server* instalados é necessário, inicialmente, instalar o docker.
A instalaçăo varia de acordo com a versăo do sistema operacional sendo utilizado (Ubuntu 14.04, 16.04, etc), portanto, pode-se acessar o link (https://docs.docker.com/install/linux/docker-ce/ubuntu/) e seguir os passos que correspondem ao seu caso.

Com o docker instalado, executar o seguinte comando no terminal:
```sh
$ git clone https://github.com/brocaar/loraserver-pi-gen.git
```

Este comando irá baixar o repositório do *Lora Server*.

Execute os comandos:

```sh
$ modprobe binfmt_misc
$ chmod 777 loraserver-pi-gen
```

Acesse a pasta *loraserver-pi-gen* e execute o comando:
```sh
$ ./build-docker.sh
```

Este irá gerar um arquivo ```.img``` com a imagem do *Raspbian* para ser utilizado no *gateway*. Esse processo poderá demorar algumas horas. A imagem é gerada na pasta ```deploy/```.

Uma segunda opçăo para geraçăo da imagem está explicada no repositório deste link (https://github.com/brocaar/lora-gateway-os). O processo é semelhante, necessitando do *docker* e gerando a imagem através de comandos simples.

Após inserir um SD Card no computador, utilize uma ferramenta qualquer, como o *Startup Disk Creator* do *Ubuntu*, para gravar a imagem no cartăo de memória.

#### Configuraçăo inicial

É necessário acessar a *Raspberry Pi* em um primeiro momento para habilitar o acesso por SSH. Insira o SD Card, um monitor via HDMI e um teclado à placa e execute os comandos:

```sh
$ sudo systemctl enable ssh
$ sudo systemctl start ssh
```

**Observação**: caso seja pedido usuário e senha =>  ```user: pi``` e ```password: raspberry```

Outro ponto seria configurar o IP da *Raspberry*. Executar o comando:
```ssh
$ sudo nano /etc/network/interfaces
```

E adicionar o seguinte ao final do arquivo:
```
auto eth0
iface eth0 inet static
    address 192.168.1.203
    netmask 255.255.255.0
```

Após salvar o arquivo, execute o comando:
```sh
$ sudo ifup -a
```
Deste modo, a *Raspberry Pi* terá IP fixo e poderá ser acessada sempre da mesma maneira.

Execute também o comando ```$ hostname -I``` e certifique que o IP retornado é igual ao que acabou de ser configurado. Salve esta informação para acessar posteriormente a placa via SSH.

#### Primeiro acesso

Feito isso, pode-se acessar a *Raspberry* de outro computador via SSH com o comando:
```sh
$ ssh pi@192.168.1.203
```

Será necessário configurar o IP da sua máquina para realizar a comunicação ssh via cabo de rede. No *Linux*, pode-se utilizar, por exemplo, os comandos:

```sh
$ ifconfig eth0 192.168.1.183 netmask 255.255.255.0 up
$ route add default gw 192.168.1.1
$ echo "nameserver 1.1.1.1" > /etc/resolv.conf
```

Utilize o comando ```$ ps -ef | grep lora``` para certificar que todos os processos estão sendo executados (loraserver, lora-app-server, lora-gateway-bridge e packet-forwarder).

**Observação**: as instruçőes abaixo citam caminhos para determinados arquivos. Versőes futuras da imagem gerada podem ter estes caminhos alterados, porém as alteraçőes que devem ser feitas são as mesmas, necessitando apenas encontrar os arquivos corretos.

O próximo passo é alterar o pino de RESET no arquivo localizado em ```/opt/semtech/packet_forwarder/lora_pkt_fwd/```. Execute:
```sh
$ sudo nano start.sh
```
E altere a linha 3 para => IOT_SK_SX1301_RESET_PIN=7

Depois é necessário setar os canais nos quais o gateway irá operar. O arquivo que deve ser alterado está em ```/opt/semtech/packet_forwarder/lora_pkt_fwd/global_conf.json```, que por *default* está configurado na faixa de 868 MHz. Renomeie este arquivo para *backup* e substitua-o pelo arquivo ```/cfg/global_conf.json.US902.basic```, renomeando-o para ```global_conf.json```. Isso pode ser feito usando os comandos:

```sh
$ sudo mv /opt/semtech/packet_forwarder/lora_pkt_fwd/global_conf.json /opt/semtech/packet_forwarder/lora_pkt_fwd/global_conf.json_bkp
$ sudo cp cfg/global_conf.json.US902.basic /opt/semtech/packet_forwarder/lora_pkt_fwd/global_conf.json
```

Além disso, copie o arquivo *global_conf.json* para a pasta ```/etc/lora-gateway-pf```. Ainda neste arquivo, altere as portas de comunicaçăo UDP (serv_port_up e serv_port_down) para 1700.

É preciso também inserir as configuraçőes de potência de transmissão das mensagens de *downlink*, também no arquivo ```global_conf.json```. Pode-se copiar e colar as linhas que contêm as *structures* ```tx_lut_n``` do arquivo de backup para este arquivo.

Além disso, é necessário fazer algumas alteraçőes no arquivo ```/etc/loraserver/loraserver.toml```, o que resultará no seguinte (copie e cole):

```
# See https://www.loraserver.io/lora-gateway-bridge/install/config/ for a full
# configuration example and documentation.

[postgresql]
dsn="postgres://loraserver_ns:loraserver_ns@localhost/loraserver_ns?sslmode=disable"

[network_server]
net_id="000000"

[network_server.band]
# LoRaWAN band to use.
#
# Valid values are:
# *	AS_923
# * AU_915_928
# * CN_470_510
# * CN_779_787
# * EU_433
# * EU_863_870
# * IN_865_867
# * KR_920_923
# * US_902_928),
name="US_902_928"

[network_server.network_settings]

# Installation margin (dB) used by the ADR engine.
#
# A higher number means that the network-server will keep more margin,
# resulting in a lower data-rate but decreasing the chance that the
# device gets disconnected because it is unable to reach one of the
# surrounded gateways.
installation_margin=10

# RX window (Class-A).
#
# Set this to:
# 0: RX1, fallback to RX2 (on RX1 scheduling error)
# 1: RX1 only
# 2: RX2 only
rx_window=0

# Class A RX1 delay
#
# 0=1sec, 1=1sec, ... 15=15sec. A higher value means LoRa Server has more
# time to respond to the device as the delay between the uplink and the
# first receive-window will be increased.
rx1_delay=1

# RX1 data-rate offset
#
# Please consult the LoRaWAN Regional Parameters specification for valid
# options of the configured network_server.band.name.
rx1_dr_offset=0

# RX2 data-rate
#
# When set to -1, the default RX2 data-rate will be used for the configured
# LoRaWAN band.
#
# Please consult the LoRaWAN Regional Parameters specification for valid
# options of the configured network_server.band.name.
rx2_dr=8

# RX2 frequency
#
# When set to -1, the default RX2 frequency will be used.
#
# Please consult the LoRaWAN Regional Parameters specification for valid
# options of the configured network_server.band.name.
rx2_frequency=923300000

# Downlink TX Power (dBm)
#
# When set to -1, the downlink TX Power from the configured band will
# be used.
#
# Please consult the LoRaWAN Regional Parameters and local regulations
# for valid and legal options. Note that the configured TX Power must be
# supported by your gateway(s).
downlink_tx_power=-1

# Disable mac-commands
#
# When set, uplink mac-commands are ignored and the network-server will not
# send mac-commands to the devices. This is intended for testing only.
disable_mac_commands=false

# Disable ADR
#
# When set, this globally disables ADR.
disable_adr=false


# Enable only a given sub-set of channels
#
# Use this when ony a sub-set of the by default enabled channels are being
# used. For example when only using the first 8 channels of the US band.
#
enabled_uplink_channels=[0,1,2,3,4,5,6,7]

```

Use o comando ```sudo systemctl restart loraserver.service``` para que as alteraçőes sejam aplicadas. Sempre que alguma mudança for feita em algum dos arquivos de configuração, um comando como este deve ser executado.

Feito isso, abra um navegador com o proxy desativado e acesse o endereço ```http://192.168.1.203:8080``` para ter acesso à interface do Lora App Server. O usuário e senha *defaults* săo ```admin```.

Na interface, é necessário criar o *network-server* (*address*=localhost:8000), *gateway-profile*, *service-profile*, *device-profile*, cadastrar o *gateway*, as aplicaçőes e finalmente cadastrar os dispositivos.

**Gateway-profile:** 
* crie um nome para o *gateway* e indique quais são os canais de frequência que este utiliza, que são os mesmos canais configurados no arquivo anterior (enabled_uplink_channels=[0,1,2,3,4,5,6,7]). Neste caso, se está indicando que a primeira sub-banda (8 primeiras frequências) da faixa de frequência de 915 MHz está sendo utilizada. É preciso também colocar o ID do *gateway*, o qual pode ser conferido executando o comando abaixo e verificando o ID presente no tópico das mensagens. Por exemplo: ```aa555a0000000101```.

```sh
$ sudo mosquitto_sub -t "#" -v
```

**Device-profile:** 
* na aba *GENERAL*, crie um nome, indique a versão do LoRaWAN utilizada (1.0.3, usada no concentrador), a revisăo (A) e a máxima potência de radiação do *gateway*.
* na aba *JOIN (OTAA / ABP)*, indique o tempo em que a primeira janela de recebimento ficará aberta (1 segundo por *default*), o *offset* da taxa de dados (0, por padrão), a taxa de dados da segunda janela de recebimento (8), a frequência da segunda janela de recebimento (923300000, por padrão) e o último campo pode ser setado em 0. O *gateway* irá utilizar estas informaçőes para efetuar o envio das mensagens de *downlink*

**Applications:** 
* crie uma nova aplicação e, dentro dela, cadastre os dispositivos passando seu endereço e chaves (podem ser setadas se o *end-device* já possuir estes dados ou geradas e setadas no *end-device*) na aba *ACTIVATION*. Na aba *CONFIGURATION* dê um nome ao dispositivo. A caixa *Disable frame-counter validation* pode ser marcada. Nas abas *LIVE DEVICE DATA* e *LIVE LORAWAN FRAMES* pode-se acompanhar o fluxo de mensagens.

Para as demais configuraçőes na interface não há mistérios, basta preencher o que é pedido.

#### Teste de *downlink* de mensagens
 
Pode-se fazer o *schedule* de mensagens específicas a serem enviadas para o *end-device* de modo a testar a comunicação. Há duas opçőes para efetuar tal tarefa:

##### 1ª opçăo
Na *Raspberry*, publique mensagens no tópico de *downlink*, as quais serão enviadas assim que uma mensagem for recebida pelo *gateway*. Um exemplo de comando a ser usado é:

```ssh
$ mosquitto_pub -h localhost -p 1883 -t application/1/device/393032355d378c05/tx -m "{\"confirmed\": false, \"fPort\": 20, \"data\": \"YmNjNTVj\"}"
```

Obs.: o campo "data" deve estar codificado no formato base64.

Caso necessário, instale o *software* para realizar a comunicação MQTT usando o comando:

```ssh
$ sudo apt-get install mosquitto mosquitto-clients
```

O tópico no qual a mensagem é publicada, passada no parâmetro ```-t```, segue o padrão ```application/[applicationID]/device/[devEUI]/tx```, portanto altere os campos [applicationID] e [devEUI] para o seu caso específico.

Referência: https://www.loraserver.io/lora-app-server/integrate/sending-receiving/mqtt/

##### 2ª opçăo
No link http://192.168.1.203/api, no campo ```/api/devices/{device_queue_item.dev_eui}/queue```, pode-se setar uma mensagem de *downlink*. Para tal, deve-se seguir as instruçőes em (https://www.loraserver.io/lora-app-server/integrate/auth/) para que a autenticação com a aplicação seja feita. Basicamente, o que deve ser feito é:

* acesse o site "https://jwt.io/"
* substitua o campo "PAYLOAD: DATA" por:
```
    {
	"iss": "lora-app-server",
	"aud": "lora-app-server",
	"nbf": 1489566958,
	"exp": 1689653358,
	"sub": "admin",
	"username": "admin"
    }
```
* acesse, na Raspberry, o arquivo ```/etc/lora-app-server/lora-app-server.toml```, copie o campo *jwt_secret* e cole no campo *your-256-bit-secret* do site indicado anteriormente.
* na janela à esquerda do site, será gerado o *token* de autenticaçăo, copie-o e cole-o na barra superior direita em "http://192.168.1.203/api".
* Try it out!

#### Configurando *scripts* não nativos

Além dos serviços já configurados no *gateway* (lora-server, lora-app-server, etc), é possível configurar outros *scripts* para serem executados contiuamente.

Inicialmente, deve-se passar para a *Raspberry* via comando ```scp``` o arquivo com o código desejado (```exemplo-lora.py```). Para tal, execute o comando:

```sh
$ sudo scp exemplo-lora.py pi@192.168.1.203:/home/pi
```

Feito isso, pode-se setar o *script* ```exemplo-lora.py``` para executar como um serviço no sistema operacional. Seguindo o tutorial disponível em http://www.diegoacuna.me/how-to-run-a-script-as-a-service-in-raspberry-pi-raspbian-jessie/, pode-se realizar os seguintes passos:

* Criar arquivo do serviço:

```sh
$ cd /lib/systemd/system/
$ sudo nano exemplo-lora.service
```

* Inserir as instruçőes do serviço:

```
[Unit]
Description=Gateway LoRaWAN
After=multi-user.target
 
[Service]
Type=simple
ExecStart=/usr/bin/python /home/pi/exemplo-lora.py
Restart=on-abort
 
[Install]
WantedBy=multi-user.target
```

* Ativaçăo do serviço:

```sh
$ sudo chmod 644 /lib/systemd/system/exemplo-lora.service
$ chmod +x /home/pi/exemplo-lora.py
$ sudo systemctl daemon-reload
$ sudo systemctl enable exemplo-lora.service
$ sudo systemctl start exemplo-lora.service
```

* Checar se serviço está rodando:

```sh
$ sudo systemctl status exemplo-lora.service
```

* Checar o *log* do serviço:

```sh
$ sudo journalctl -f -u exemplo-lora.service
```

O mesmo procedimento pode ser realizado com outros *scripts* que necessitam serem executados continuamente.

#### Referências adicionais

* https://www.loraserver.io/
* https://github.com/brocaar/loraserver-pi-gen
* https://docs.docker.com/install/linux/docker-ce/ubuntu/
