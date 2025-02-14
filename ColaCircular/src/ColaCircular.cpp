#include <mutex>
#include <cstdio>
#include <memory>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <string.h> /* for strncpy */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <netinet/in.h>
#include <net/if.h>


template <class T>
class cola_circular {
public:
	cola_circular(size_t tamano) :							//El constructor llama a los constructores delegados
		cola(std::unique_ptr<T[]>(new T[tamano])),			//Puntero inteligente hacia el espacio asignado a la cola, cumple con patron RAII
		tamano_max(tamano)
{
}

	T get()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if(is_vacia())		//Si esta vacía la cola devuelvo un objeto vacío.
		{
			return T();
		}
		//Leo el dato al final de la cola y avanzo el final una posición, queda una posición mas vacía.
		auto val = cola[final];
		completo = false;
		final = (final + 1) % tamano_max;

		return val;
	}

	void put(T objeto)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		cola[inicio] = objeto;			//Guardo el objeto al inicio de la cola

		if(completo)					//Si esta llena la cola se guarda en la posición final y se avanza el final
		{
			final = (final + 1) % tamano_max;
		}
		inicio = (inicio + 1) % tamano_max;		//Avanzo el inicio
		completo = inicio == final;				//Si al guardar el inicio es igual al final entonces la cola esta llena
	}

	bool is_llena() const
	{
		return completo;
	}

	bool is_vacia() const
	{
		//Si inicio y final son iguales y no esta completo es porque esta vacío
		return (!completo && (inicio == final));
	}

	size_t get_tamano() const
	{
		return tamano_max;
	}

	size_t get_ocupado() const		//Espacio de la cola ocupado
	{
		size_t ocupado = tamano_max;
		if(!completo)
		{
			if(inicio >= final)
			{
				ocupado = inicio - final;
			}
			else
			{
				ocupado = tamano_max + inicio - final;
			}
		}
		return ocupado;
	}

	void reinicio()
	{
		std::lock_guard<std::mutex> lock(mutex_);
		completo = false;
		inicio = final;
	}

private:
	std::mutex mutex_;
	std::unique_ptr<T[]> cola;
	const size_t tamano_max;
	bool completo = 0;
	size_t inicio = 0;
	size_t final = 0;
};


cola_circular<uint32_t> cola(3);		//Cola de tamaño 10 global


void* horaRTC (void* arg){
	char fecha_hora[256];
	unsigned long irq_frec;
	int fd,c;
	struct rtc_time tm;

	while(1){
		if(!cola.is_vacia()){
			c=cola.get();
			if (c==1){
				printf("\nAcceso al dispositivo RTC via ioctl()\n");
				/*
				 * Leer fecha y hora del RTC
				 */
				if ((fd = open("/dev/rtc", O_RDONLY, S_IREAD)) < 0){
					printf("Error: no se puede abrir /dev/rtc\n");
					return NULL;
				} else {
					if ((ioctl(fd, RTC_RD_TIME, &tm)) != 0){
						printf("Error: ioctl sobre /dev/rtc\n");
						close(fd);
						return NULL;
					} else {
						printf("Hora: %02d:%02d:%02d\n", tm.tm_hour,
								tm.tm_min, tm.tm_sec);
					}
				}
				close (fd);
			}else{
				cola.put(c);	//Si no era para este thread vuelvo a colocarlo en cola
			}
		}
		sleep(1);
	}
	return NULL;
}


void* fechaRTC(void* arg){
	char fecha_hora[256];
	unsigned long irq_frec;
	int fd,c;
	struct rtc_time tm;

	while(1){
		if(!cola.is_vacia()){
			c=cola.get();
			if (c==2){
				printf("\nAcceso al dispositivo RTC via ioctl()\n");
				/*
				 * Leer fecha y hora del RTC
				 */
				if ((fd = open("/dev/rtc", O_RDONLY, S_IREAD)) < 0){
					printf("Error: no se puede abrir /dev/rtc\n");
					return NULL;
				} else {
					if ((ioctl(fd, RTC_RD_TIME, &tm)) != 0){
						printf("Error: ioctl sobre /dev/rtc\n");
						close(fd);
						return NULL;
					} else {
						printf("fecha: %02d/%02d/%02d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
					}
				}
				close (fd);
			}else{
				cola.put(c);	//Si no era para este thread vuelvo a colocarlo en cola
			}
		}
		sleep(1);
	}
	return NULL;
}

void* iPv4 (void* arg){
	int fd,c;
	struct ifreq pred;
	struct sockaddr_in *ipaddr;
	char address[INET_ADDRSTRLEN];

	while(1){
		if(!cola.is_vacia()){
			c=cola.get();
			if (c==3){
				strncpy(pred.ifr_name, "wlp2s0", IFNAMSIZ-1);

				fd = socket(AF_INET, SOCK_DGRAM, 0);
				if (fd < 0) {
					printf("Error al establecer el socket \n");
					return NULL;
				}

				printf("\nLos parámetros de la interfaz son:\n\n");

				if (ioctl(fd, SIOCGIFADDR, &pred) == -1) {
					printf("Error al obtener la IP\n");
					close(fd);
					return NULL;
				}

				ipaddr = (struct sockaddr_in *)&pred.ifr_addr;
				if (inet_ntop(AF_INET, &ipaddr->sin_addr, address, sizeof(address)) != NULL) {
					printf("Ip address: %s\n", address);
				}
			}else{
				cola.put(c);	//Si no era para este thread vuelvo a colocarlo en cola
			}
		}
		sleep(1);
	}
	return NULL;
}

void* interfaz(void* arg){
	int opcion;
	while(1){

		printf ("Opciones\n");
		printf ("1. Hora de Rtc\n");
		printf ("2. Fecha del sistema\n");
		printf ("3. Dirección IP de la computadora\n");
		scanf("%d",&opcion);

		cola.put(opcion);
		printf("Opción ingresada: %d\n", opcion);

		sleep(1);
	}

	return NULL;
}

int main(void)
{
	pthread_t hilo0, hilo1, hilo2,hilo3;
	printf("Ocupado: %zu, Tamaño total: %zu\n", cola.get_ocupado(), cola.get_tamano());

	pthread_create(&hilo0, NULL, interfaz, NULL);
	pthread_create(&hilo1, NULL, horaRTC, NULL);
	pthread_create(&hilo2, NULL, fechaRTC, NULL);
	pthread_create(&hilo3, NULL, iPv4, NULL);

	while(1){
		sleep(2);
	}

	return 0;
}
