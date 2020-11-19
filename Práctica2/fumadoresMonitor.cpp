#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

mutex mtx;
std::vector<std::string> ingredientes{"cerillas", "tabaco", "papel"};
//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}


int producirIngrediente(){
  int value = aleatorio<0,2>();
  mtx.lock();
  cout << "Se ha producido el ingrediente " << value << " (" << ingredientes[value] << ")"<< endl << flush;
  mtx.unlock();
  return value;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

class Estanco : public HoareMonitor{
  private:
    static const int num_fumadores = 3;
    CondVar mostr_vacio,
            fumador[num_fumadores];
    int ingr_disp;
  public:
    Estanco();
    void ponerIngrediente(int ingr);
    void obtenberIngrediente(int num_f);
    void esperarRecogidaIngrediente();
};

Estanco::Estanco(){
  mostr_vacio = newCondVar();
  for(int i = 0; i < 3; i++){
    fumador[i] = newCondVar();
  }
  ingr_disp = -1;
}
//----------------------------------------------------------------------

void Estanco::ponerIngrediente(int ingr){
  ingr_disp = ingr;
  mtx.lock();
  cout << "Se ha puesto ingrediente " << ingredientes[ingr_disp] << endl << flush;
  mtx.unlock();
  fumador[ingr_disp].signal();
}

//----------------------------------------------------------------------

void Estanco::obtenberIngrediente(int num_f){
  if(num_f != ingr_disp)
    fumador[num_f].wait();

    ingr_disp = -1;

    mostr_vacio.signal();
}

//----------------------------------------------------------------------

void Estanco::esperarRecogidaIngrediente(){
  if(ingr_disp!=-1)
    mostr_vacio.wait();
}

//----------------------------------------------------------------------

int ProducirIngrediente(){
  int ingrediente = aleatorio<0,2>();
  this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
  mtx.lock();
  cout << "Se ha producido el ingrediente " << ingredientes[ingrediente] << endl << flush;
  mtx.unlock();
  return ingrediente;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void funcion_hebra_fumador( MRef<Estanco>estanco, int num_fumador ){
  mtx.lock();
  std::cout << "Fumador " << num_fumador << " necesita ingrediente " << ingredientes[num_fumador] << std::endl;
  mtx.unlock();
   while( true ){
     estanco->obtenberIngrediente(num_fumador);
     mtx.lock();
     cout << "Retirado ingrediente: " << ingredientes[num_fumador] << " por el fumador " << num_fumador << endl;
     mtx.unlock();
     fumar(num_fumador);
   }
}


//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero

void funcion_hebra_estanquero( MRef<Estanco>estanco ){
  int ingrediente;
  while(true){
    ingrediente = producirIngrediente();
    estanco->ponerIngrediente(ingrediente);
    estanco->esperarRecogidaIngrediente();
  }
}

//----------------------------------------------------------------------

int main(){
   cout << "Fumadores con monitor " << endl << flush;
   MRef<Estanco> estanco = Create<Estanco>();
   std::thread fumadores[3];
   std::thread estanquero(funcion_hebra_estanquero, estanco);
   for(int i = 0; i < 3; i++) fumadores[i] = thread(funcion_hebra_fumador, estanco, i);
   for(int i = 0; i < 3; i++) fumadores[i].join();
   estanquero.join();
}
