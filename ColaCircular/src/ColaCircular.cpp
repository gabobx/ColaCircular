#include <mutex>
#include <cstdio>
#include <memory>

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

	size_t get_ocupado() const		//Espacio de la cola get_ocupado
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

int main(void)
{
	cola_circular<uint32_t> cola(10);		//Creo una cola de tamaño 10
	printf("Ocupado: %zu, Tamaño total: %zu\n", cola.get_ocupado(), cola.get_tamano());

	for (int var = 0; var < 10; ++var) {
		printf("Guardo en cola el valor: %d\n", var);
		cola.put(var);
	}
	printf("Cola llena: %d\n", cola.is_llena());
	int x = cola.get();
	printf("Leo primer valor guardado: %d\n", x);
	printf("Guardado en cola el valor 18 \n");
	cola.put(18);
	x = cola.get();
	printf("Leo segundo valor guardado: %d\n", x);
	while(!cola.is_vacia()){
		x = cola.get();
		printf("Leo siguiente valor guardado: %d\n", x);
	}
	printf("Cola vacia: %d\n", cola.is_vacia());

	return 0;
}
