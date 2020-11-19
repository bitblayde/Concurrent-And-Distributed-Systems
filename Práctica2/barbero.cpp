#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

constexpr auto num_clientes = 10;
mutex mtx;

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}


class Barberia : public HoareMonitor{
  private:
    CondVar barbero,
            silla,
            sala_espera;
    int clientes;
  public:
    Barberia();
    ~Barberia() = default;
    void cortarPelo(int cliente);
    void siguienteCliente();
    void finCliente();
};

Barberia::Barberia(){
  clientes = 0;
  barbero = newCondVar();
  silla = newCondVar();
  sala_espera = newCondVar();
}

//  Funciones de la Barberia
void Barberia::cortarPelo(int cliente){
  clientes++;
  mtx.lock();
  cout << "El cliente " << cliente << " entra a la barberia" << endl << flush;
  mtx.unlock();
  
  if(!barbero.empty()){
    mtx.lock();
    cout << "El cliente " << cliente << " despierta al barbero" << endl << flush;
    mtx.unlock();
    barbero.signal();
  }
  else  {
    if(clientes > 1){
      mtx.lock();
      cout << "El cliente " << cliente << " va a la sala de espera" << endl << flush;
      mtx.unlock();
      sala_espera.wait();
    }
  }

  mtx.lock();
  cout << "El cliente " << cliente << " se está pelando " << endl << flush;
  mtx.unlock();

  silla.wait();
}

void Barberia::siguienteCliente(){
  if(clientes == 0){
    mtx.lock();
    cout << "Barbero durmiendo " << endl << flush;
    mtx.unlock();
    barbero.wait();
  }
  if(silla.empty()){
    mtx.lock();
    cout << "Se llama al siguiente cliente " << endl << flush;
    mtx.unlock();
    sala_espera.signal();
  }
}

void Barberia::finCliente(){
  mtx.lock();
  cout << "FIN cliente " << endl << flush;
  mtx.unlock();
  silla.signal();
  clientes--;
}
//  END. Funciones de la Barberia

void EsperarFueraBarberia(unsigned cliente){
  mtx.lock();
  cout << "El cliente " << cliente << " espera fuera de la barberia " << endl << flush;
  mtx.unlock();
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
}

void CortarPeloACliente(){
  mtx.lock();
  cout << "Se está cortando el pelo al cliente " << endl << flush;
  mtx.unlock();
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
}


void funcion_hebra_cliente(MRef<Barberia>barberia, unsigned num_cliente){
  while(true){
    barberia->cortarPelo(num_cliente);
    EsperarFueraBarberia(num_cliente);
  }
}

void funcion_hebra_barbero(MRef<Barberia>barberia){
  while(true){
    barberia->siguienteCliente();
    CortarPeloACliente();
    barberia->finCliente();
  }
}

int main(void){
  cout << "-----------------Barberia-----------------------" << endl << flush;
  thread clientes[num_clientes],
  barbero;

  MRef<Barberia> barberia = Create<Barberia>();
  barbero = thread(funcion_hebra_barbero, barberia);

  for(int i = 0; i < num_clientes; i++){
    clientes[i] = thread(funcion_hebra_cliente, barberia, i);
  }

  barbero.join();
  for(int i = 0; i < num_clientes; i++){
    clientes[i].join();
  }
}
