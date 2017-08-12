import math
import numpy as np

## Parâmetros
ht = 40; # height of BS antenna
hr = 3;  # height of mobile antenna
Gt = 1;  # BS antenna gain
Gr = 1;  # Mobile antenna gain
c = 3*10**8
f = 1800*10**6; # in Hz
lamda = c / f;
d0 = 1000; # in metres
P0 = 10**-6; 

n1 = 3;
n2 = 4;
distances = range(10**3, 20*10**3);

## Modelo espaço livre
receivedPower = []
j = 0

for i in distances:
	Pr = 10*np.log10(P0/(10**-3)) + 20*np.log10(d0/distances[j])
	receivedPower.append(Pr)
	j += 1


from matplotlib import pyplot
pyplot.plot(distances, receivedPower)
pyplot.xlabel('Distância (m)')
pyplot.ylabel('Potência recebida (dBm)')
pyplot.title('Modelo espaço livre')
pyplot.grid(True, which="both", ls="--")

pyplot.xscale('log')
# pyplot.yscale('log')

pyplot.show()
## Fim do modelo espaço livre

## Modelo log-distância
receivedPowerLogDistancia1 = []
receivedPowerLogDistancia2 = []
j = 0

for i in distances:
	Pr = 10*np.log10(P0/(10**-3)) + 10*n1*np.log10(d0/distances[j])
	receivedPowerLogDistancia1.append(Pr)
	Pr = 10*np.log10(P0/(10**-3)) + 10*n2*np.log10(d0/distances[j])
	receivedPowerLogDistancia2.append(Pr)
	j += 1

pyplot.plot(distances, receivedPowerLogDistancia1)
pyplot.xlabel('Distância (m)')
pyplot.ylabel('Potência recebida (dBm)')
pyplot.title('Modelo Log Distância para n = 3')
pyplot.grid(True, which="both", ls="--")

pyplot.xscale('log')
# pyplot.yscale('log')

pyplot.show()

pyplot.plot(distances, receivedPowerLogDistancia2)
pyplot.xlabel('Distância (m)')
pyplot.ylabel('Potência recebida (dBm)')
pyplot.title('Modelo Log Distância para n = 4')
pyplot.grid(True, which="both", ls="--")

pyplot.xscale('log')
# pyplot.yscale('log')

pyplot.show()
## Fim do modelo log-distância

## Modelo de Hata estendido
Pt = 10*np.log10(P0) + 46.3 + 33.9*np.log10(f) - 13.82*np.log10(ht) - hr*(1.1*np.log10(f) - 0.7) + (1.56*np.log10(f) - 0.8) + np.log10(d0)*(44.9 - 6.55*np.log10(ht)) # em dB

receivedPower = []
j = 0

for i in distances:
	Pr = Pt - (46.3 + 33.9*np.log10(f) - 13.82*np.log10(ht) - hr*(1.1*np.log10(f) - 0.7) + (1.56*np.log10(f) - 0.8) + np.log10(distances[j])*(44.9 - 6.55*np.log10(ht))) # em dB
	Pr += 30 # em dBm 
	receivedPower.append(Pr)
	j += 1

pyplot.plot(distances, receivedPower)
pyplot.xlabel('Distância (m)')
pyplot.ylabel('Potência recebida (dBm)')
pyplot.title('Modelo Hata estendido')
pyplot.grid(True, which="both", ls="--")

pyplot.xscale('log')
# pyplot.yscale('log')

pyplot.show()
## Fim do modelo de Hata estendido