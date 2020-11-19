// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// archivo: barrera2_su.cpp
// Ejemplo de un monitor 'Barrera' parcial, con semántica SU
//
// Historial:
// Creado en Julio de 2017
// -----------------------------------------------------------------------------


#include <iostream>
#include <iomanip>
#include <random>
#include <thread>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;



constexpr int num_items = 40,
              np = 8,
              nc = 5;

int Array_p[np]={0},
    Array_c[nc]={0};

mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items] = {0}, // contadores de verificación: producidos
   cont_cons[num_items] = {0}; // contadores de verificación: consumidos

   //**********************************************************************
   // funciones comunes a las dos soluciones (fifo y lifo)
   //----------------------------------------------------------------------

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

   int producir_dato(int num_thread)
   {
       int contador = Array_p[num_thread]+num_thread*num_items/np ;
      this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
      mtx.lock();
      cout << "producido: " << contador << " Hebra " << num_thread << endl << flush ;
      mtx.unlock();
      cont_prod[contador] ++ ;
      Array_p[num_thread]++;
      return contador++ ;
   }
   //----------------------------------------------------------------------

   void consumir_dato( unsigned dato, int num_thread )
   {
      if ( num_items <= dato )
      {
         cout << " dato === " << dato << ", num_items == " << num_items << endl ;
         assert( dato < num_items );
      }
      cont_cons[dato]++;
      Array_c[num_thread]++;
      this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
      mtx.lock();
      cout << "                  consumido: " << dato << " por hebra " << num_thread << endl ;
      mtx.unlock();
   }
//----------------------------------------------------------------------


void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}


// *****************************************************************************
// clase para monitor Barrera, version 2,  semántica SU

class prodconsLIFO : public HoareMonitor
{
   private:
   static const int num_celdas_total = 10; // tamaño del vector
   int      buffer[num_celdas_total],
            primera_libre;

   CondVar  libres,            // cola de hebras esperando en cita
            ocupadas;


   public:
   prodconsLIFO( ) ; // constructor
   int leer();
   void escribir(int value);
} ;
// -----------------------------------------------------------------------------

prodconsLIFO::prodconsLIFO( )
{
   primera_libre = 0;
   libres          = newCondVar();
   ocupadas        = newCondVar();
}
// -----------------------------------------------------------------------------

int prodconsLIFO::leer(){
  if(primera_libre == 0)
    ocupadas.wait();

    assert(primera_libre > 0);

    primera_libre--;
    const int valor = buffer[primera_libre];

    libres.signal();
    return valor;
}

void prodconsLIFO::escribir(int value){
  if(primera_libre == num_celdas_total)
    libres.wait();

    assert(primera_libre < num_celdas_total);

    buffer[primera_libre] = value;
    primera_libre++;
    ocupadas.signal();

}

void funcion_hebra_productora(MRef<prodconsLIFO> monitor, int num_hebra){
  for(int i = 0; i < num_items/np; i++){
    int valor = producir_dato(num_hebra);
    monitor->escribir(valor);
  }
}

void funcion_hebra_consumidora(MRef<prodconsLIFO> monitor, int num_hebra){
  for(int i = 0; i < num_items/nc; i++){
    int valor = monitor->leer();
    consumir_dato(valor, num_hebra);
  }
}
// -----------------------------------------------------------------------------
/*
void funcion_hebra( MRef<prodconsLIFO> monitor, int num_hebra )
{
   while( true )
   {
      const int ms = aleatorio<0,30>();
      this_thread::sleep_for( chrono::milliseconds(ms) );
      monitor->cita( num_hebra );
   }

} */
// *****************************************************************************

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

int main()
{
   cout <<  "Barrera parcial SU LIFO: inicio simulación." << endl << flush ;

   /*// declarar el número total de hebras
   const int num_hebras      = 100,
             num_hebras_cita = 10 ; */

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   thread hebra_productora[np],
          hebra_consumidora[nc];


   MRef<prodconsLIFO> monitor = Create<prodconsLIFO>(  );


   ini_contadores();

   for(int i = 0; i < np; i++){
     //Array_p[i] = 0;
     hebra_productora[i] = thread(funcion_hebra_productora, monitor, i);
   }

   for(int i = 0; i < nc; i++){
     //Array_c[i] = 0;
     hebra_consumidora[i] = thread(funcion_hebra_consumidora, monitor, i);
   }

   for(int i = 0; i < np; i++){
     hebra_productora[i].join();
   }

   for(int i = 0; i < nc; i++){
     hebra_consumidora[i].join();
   }

   // comprobar que cada item se ha producido y consumido exactamente una vez
   test_contadores() ;
}
