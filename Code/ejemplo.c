#include <gb/gb.h>
#include <stdio.h>
#include "Tile1.c"
#include "Tile2.c"
#include "Mapa.c"
#include "Nivel1.c"
#include "Nivel1objs.c"
#include "Nivel2.c"
#include "Nivel2objs.c"

#define MAX_OBJS     3
#define MAX_MONEDAS     10


//Para pasar al segundo nivel al tocar la puerta del nivel 1
unsigned char* ListaNiveles[]=
{
    Nivel1Label, Nivel2Label,
};

unsigned char* ListaNivelesObjetos[]=
{
    Nivel1objsLabel, Nivel2objsLabel,
};


struct Objeto{
    UBYTE spritids[6];
    UINT8 x,y, act, tipo, dir; //datos de 0 a 255
    //act es para indicar si esta activo, en la pantalla, tipo de enemigo
    //en que dirección apuntan los enemigos
    signed vy; //caida vertical
};




//-------------- NO HAGAS VARIABLES GLOBALES, HAZLAS TODAS LOCALES  --------------


//posicion del jugador
unsigned char plx,ply;
//Posixiones antiguas jugador para desplazar el fondo (mover la camara)
unsigned oldplx,pldir;
signed char scrollX;

//camara
unsigned char camx;

//variables para la gravedad
signed char plvy;
unsigned char A_pressed;
unsigned char vuelveSaltar; //para no saltar indefinidamente

struct Objeto jugador;

//Enemigos 
struct Objeto enemigo[MAX_OBJS];

unsigned int i; //identificar el enemigo correpsondiente que se va a cargar
unsigned char en; //calcular cuantos enemigos hay en pantalla

//Balas para matar enemigos
struct Objeto bala[MAX_OBJS];
unsigned char B_pressed;
unsigned int j; //para calcular las colisiones de balas con enemigos
unsigned char b; //administras el control de las balas

//Monedas
unsigned char m; //representa las monedas
struct Objeto moneda[MAX_MONEDAS];
unsigned char mn; //para guardar el número de monedas coleccionadas hasta ahora

//Animación del personaje
unsigned char animFrame; //sirve para contar los fotogramas de animación

//Animación del enemigo
unsigned char enemFrame;

//niveles
unsigned char nivel_id; //nivel actual

void moverObjeto(struct Objeto* obj, UINT8 x, UINT8 y)
{
    move_sprite(obj->spritids[0],x,y);
    move_sprite(obj->spritids[1],x+8,y);
    move_sprite(obj->spritids[2],x,y+8);
    move_sprite(obj->spritids[3],x+8,y+8);
    move_sprite(obj->spritids[4],x,y+16);
    move_sprite(obj->spritids[5],x+8,y+16);
}

void setupBalas()
{
    //el array es para que no se pase de 3 balas
    set_sprite_tile(15,13);
    bala[b%MAX_OBJS].spritids[0]=15; //aquí la bala está normal (15 es el id y 13 el sprite)
    set_sprite_tile(16,13);
    bala[b%MAX_OBJS].spritids[1]=16;    //aquí la bala está al revés (16 es el id)
    set_sprite_prop(16, 32); 

    bala[b%MAX_OBJS].act=1;

    bala[b%MAX_OBJS].x = plx;
    bala[b%MAX_OBJS].y = ply;

    bala[b%MAX_OBJS].dir = pldir;
}

void setupJugador()
{
    //obtener la posición del mapa
	plx=((i%32)<<3)+8;
    ply=((i>>5)<<3)+16;
    oldplx=plx;
	
	pldir=1;
	
    jugador.x = plx;
    jugador.y = ply;
    //estableces el id del sprite para la gameboy y luego el tile de donde lo sacas
    set_sprite_tile(1,15);
    jugador.spritids[0]=1; //le das el mismo id para recordar qe id lleva
    set_sprite_tile(2,16);
    jugador.spritids[1]=2;
    set_sprite_tile(3,17);
    jugador.spritids[2]=3;
    set_sprite_tile(4,18);
    jugador.spritids[3]=4;
    set_sprite_tile(17,19);
    jugador.spritids[4]=17;
    set_sprite_tile(18,20);
    jugador.spritids[5]=18;
}



void setupEnemigo()
{
    enemigo[en].x = ((i%32)<<3)+8;
    enemigo[en].y = ((i>>5)<<3)+16;
    enemigo[en].act = 1; //activarlo

    switch (enemigo[en].tipo) //dos tipos
    {
        case 1:
            set_sprite_tile(7,7);
            enemigo[en].spritids[0]=7;
            set_sprite_tile(8,8);
            enemigo[en].spritids[1]=8;
            set_sprite_tile(9,9);
            enemigo[en].spritids[2]=9;
            set_sprite_tile(10,10);
            enemigo[en].spritids[3]=10;
        break;

        case 2: //rebotar
            set_sprite_tile(11,11);
            enemigo[en].spritids[0]=11;
            set_sprite_tile(12,11);
            enemigo[en].spritids[1]=12;
            set_sprite_tile(13,12);
            enemigo[en].spritids[2]=13;
            set_sprite_tile(14,12);
            enemigo[en].spritids[3]=14;

            set_sprite_prop(12, 32);
            set_sprite_prop(14, 32);    
        break;
    }
}

//Modifica la posición y lo activa automáticamente
void setupMoneda()
{
    moneda[m].x = (i%32)<<3;
    moneda[m].y = (i>>5)<<3;
    moneda[m].act = 1;
}



void cargarObjetos() //minuto 34
{
    en=0;
	m=0; //para que reinicie a 0 las monedas al pasar de nivel

    for(i=0;i<512;++i)
    {
        if ((ListaNivelesObjetos[nivel_id])[i]==7)//Personaje principal
        //si encuentra un sprite que es el 7, el de cargar el jugador, lo carga
        {
            setupJugador();
        }
        if ((ListaNivelesObjetos[nivel_id])[i]==8)//Enemigo1
        //si encuentra un sprite que es el 8, el de cargar el jugador, lo carga
        {
            enemigo[en].tipo=1; setupEnemigo(); ++en;
        }
        if ((ListaNivelesObjetos[nivel_id])[i]==9)//Enemigo2
        {
            enemigo[en].tipo=2; setupEnemigo(); ++en;
        }
		if ((ListaNivelesObjetos[nivel_id])[i]==1)//monedas
        {
            setupMoneda(); ++m; //++m para que continue con la siguiente moneda
        }
    }
}

void matarObjeto(struct Objeto* obj)
{
    //hace que los enemigos o las balas dejen de aparecer en pantalla 
    obj->x = 0;
    obj->y = 0;
    obj->act = 0; //que no está activo

    moverObjeto(obj, 0, 0);
}


UBYTE colision_en(UINT8 newx, UINT8 newy)
{
    UINT16 indexTLx, indexTLy, tileindexTL;
    UBYTE resultado;
    //coordenada x se divide la posicion entre 8 (desplazamiento 3 bits) ya que cada tile son 8 bits
    //se toma los ultimos 5 bits que tendrán la posición
    indexTLx = (newx>>3)&31;
    //coordenada y se divide la posicion entre 8 (desplazamiento 3 bits) ya que cada tile son 8 bits
    //se multiplica por 32 (2^5) porque cada fila en el mapa nivel1 son 32 tiles
    indexTLy = (newy>>3)<<5;

    //se suman para obtener el indice en el array de tiles del nivel1, y ver si en ese tile hay algo solido
    tileindexTL = indexTLy + indexTLx;

    //se comprueba si hay un tile en el mapa con id mayor que dos, el id es la posicion en las que se ha cargado los tiles en el mapa
    resultado = (ListaNiveles[nivel_id])[tileindexTL]>=2;

    return resultado;
}

void animarEnemigo()
{
    ++enemFrame;

    if (enemFrame&8)
    { //igual que el de animar al personaje principal, de hecho es con los mismos sprites
        set_sprite_tile(7+!enemigo[i].dir,0);
        set_sprite_tile(8-!enemigo[i].dir,0);
        set_sprite_tile(9+!enemigo[i].dir,5);
        set_sprite_tile(10-!enemigo[i].dir,6);
    }
    else
    {
        set_sprite_tile(7+!enemigo[i].dir,7);
        set_sprite_tile(8-!enemigo[i].dir,8);
        set_sprite_tile(9+!enemigo[i].dir,9);
        set_sprite_tile(10-!enemigo[i].dir,10);
    }
}

void actualizarEnemigo()
{
    for (i=0;i<en;++i) //numero de enemigos
    {
        if (enemigo[i].act) //si esta activado
        {
            switch (enemigo[i].tipo) 
            {
                case 1: //ENEMIGO 1
                    if (!enemigo[i].dir) //dependiendo de la direccion se mueve a un lado u otro
                    {
                        --enemigo[i].x;
                        //para la animación del enemigo
                        set_sprite_prop(7, 32);
                        set_sprite_prop(8, 32);
                        set_sprite_prop(9, 32);
                        set_sprite_prop(10, 32);

                        animarEnemigo();
                    }
                    else if (enemigo[i].dir) 
                    {
                        ++enemigo[i].x;
                        set_sprite_prop(7, 0);
                        set_sprite_prop(8, 0);
                        set_sprite_prop(9, 0);
                        set_sprite_prop(10, 0);

                        animarEnemigo();
                    }

                    if (enemigo[i].vy>2) enemigo[i].vy=2; else ++enemigo[i].vy; //aqui que caiga

                    //si colisiona que cambie de direccion
                    if (colision_en(enemigo[i].x-8,enemigo[i].y-1)||enemigo[i].x<8) enemigo[i].dir=1;
                    if (colision_en(enemigo[i].x+8,enemigo[i].y-1)||enemigo[i].x>256-8) enemigo[i].dir=0;
                    
                    //cuando toca el suelo que no caiga mas
                    if (colision_en(enemigo[i].x-7,enemigo[i].y)||colision_en(enemigo[i].x+7,enemigo[i].y)) enemigo[i].y=(enemigo[i].y>>3)<<3;
                    else enemigo[i].y+=enemigo[i].vy;
                break;

                case 2: //ENEMIGO 2
                    if (enemigo[i].vy>2) enemigo[i].vy=2; else ++enemigo[i].vy;

                    // si cae al suelo que rebote
                    if (colision_en(enemigo[i].x,enemigo[i].y)||colision_en(enemigo[i].x+7,enemigo[i].y))
                    {
                        --enemigo[i].y;
                        enemigo[i].vy=-10;
                    }
                    else enemigo[i].y+=enemigo[i].vy;
                break;
            }

            moverObjeto(&enemigo[i], enemigo[i].x-camx, enemigo[i].y); //actualizar posicion de los enemigos
        }
        else matarObjeto(&enemigo[i]); //si no está activo lo mandamos a desaparecer porque está muerto
    }
}

UBYTE colisionObjetos(struct Objeto* uno, struct Objeto* dos)
{
    //si dos objetos han colisionado o no
    return (uno->x >= dos->x && uno->x <= dos->x + 8) && (uno->y >= dos->y && uno->y <= dos->y + 8)
    || (dos->x >= uno->x && dos->x <= uno->x + 8) && (dos->y >= uno->y && dos->y <= uno->y + 8);
}

//
void actualizarBalas()
{
    for (i=0;i<MAX_OBJS;++i)
    {
        if (bala[i].act)
        {
            if (bala[i].dir) bala[i].x+=4; else bala[i].x-=4; //si está activa que se mueva según la posición de nuestro personaje

            for (j=0;j<MAX_OBJS;++j)
            {
                if (colisionObjetos(&bala[i],&enemigo[j])) //si hay colisión de un enemigo y la bala se desactivan los dos
                {
                    enemigo[j].act=0;
                    bala[i].act=0;
					
					NR10_REG = 0x1E;
                    NR11_REG = 0x80;
                    NR12_REG = 0x43;
                    NR13_REG = 0x73;
                    NR14_REG = 0x86;
                }
            }
            //si la bala se sale del nivel (de pantalla) desaparece
            if (bala[i].x<8||bala[i].x>256-8) bala[i].act=0;
            moverObjeto(&bala[i], bala[i].x-camx, bala[i].y);
        }
        //si tras todo eso sigue activa matamos al enemigo
        else matarObjeto(&bala[i]);
    }
}

void actualizarMonedas()
{
    for (i=0;i<MAX_MONEDAS;++i)
    {
        //Si una moneda colisiona con el jugador deja de estar activa y se borra del fondo (las monedas están pintadas en el fondo)
        if (colisionObjetos(&jugador,&moneda[i]) && moneda[i].act) 
        {
            moneda[i].act=0;
            set_bkg_tiles(moneda[i].x>>3,moneda[i].y>>3,1,1,0);
			
			++mn; //indica que ya se ha coleccinado una moneda y lo indica en los mensajes
			
            //sonido de coger una moneda
			NR21_REG = 0x80;
            NR22_REG = 0x43;
            NR23_REG = 0x6D;
            NR24_REG = 0x86;
        }
    }
}

//array con elementos que representan los sprites para la palabra monedas
unsigned char Mensaje[] =
{
    36,38,37,28,27,24,42,17,
};

//los elementos son los sprites que representan los numeros del 0 al 9
unsigned char Numeros[] =
{
    7,8,9,10,11,12,13,14,15,16,
};

//al coleccionar una moneda se actualizan los números
void actualizarMensaje()
{
    set_win_tiles(9,1,1,1,Numeros+((mn/10)%10));
    set_win_tiles(10,1,1,1,Numeros+(mn%10));
}


void animarPersonaje()
{
    ++animFrame;

    //Al avanzar los fotogramas se actualizan los sprites
    if (animFrame&4)
    {
        set_sprite_tile(1+!pldir,15);    //estos están a 0 como que no se van a ver
        set_sprite_tile(2-!pldir,16);
        set_sprite_tile(3+!pldir,21);
        set_sprite_tile(4-!pldir,22);
        set_sprite_tile(17,23);
        set_sprite_tile(18,24);
    }
    else
    {
        set_sprite_tile(1+!pldir,15);
        set_sprite_tile(2-!pldir,16);
        set_sprite_tile(3+!pldir,17);
        set_sprite_tile(4-!pldir,18);
        set_sprite_tile(17,19);
        set_sprite_tile(18,20);
    }
}

//Al pasar al siguiente nivel reinicia nuestra posición y la de los nuevos enemigos
void cargarDatos()
{
    set_sprite_data(0,25,TileLabel2);
    set_sprite_tile(0,0);

    plx=16; ply=16;
    oldplx=plx;

    cargarObjetos();

    set_bkg_tiles(0,0,32,16,ListaNiveles[nivel_id]);
}

void main(){

    //elegir nivel:
    nivel_id = 0;

    A_pressed = 0;
    B_pressed = 0;
    vuelveSaltar = 0;
    
    //Para la agregación de sonidos
    NR52_REG = 0x80;    //activar el sonido
    NR50_REG = 0x77;    //ajuste de volumen (max = 77)
    NR51_REG = 0xFF;    //máximo de sonidos que podemos usar (FF = 4)

    cargarDatos();

    //cargar sprites del sprite numero 0 al sprite numero 14+1 nombre del array de sprite
    //set_sprite_data(0,15,TileLabel2);
    // capa 0 sprite 0
    //set_sprite_tile(0,0);
    // sprite 0 posx y posy en pantalla
    //mostrar sprites en pantalla
    SHOW_SPRITES;
    SHOW_BKG;
    SHOW_WIN;

    //Cargar los datos para el fondo
	set_bkg_data(0,54,MapaLabel);
    //Cargar los tiles y establecerlos como fondo
    set_bkg_tiles(0,0,32,16,Nivel1Label);

    //Win es la capa del fondo
    set_win_tiles(1,1,8,1,Mensaje);
    move_win(7,120);    //donde queremos que se vea el mensaje

    while(1)
	{
        //oldplx = plx;
        jugador.x = plx;
        jugador.y = ply;

        actualizarEnemigo();
        actualizarBalas();
        actualizarMonedas();
        actualizarMensaje();

	    if (joypad() & J_LEFT)
        {
            if(plx>8 && (!colision_en(plx-8,ply-1))) //que no se salga de la pantalla y que no se choque con algo a la izquierda
                --plx;
            pldir = 0; //direccion izquierda
            //damos la vulta al pesonaje
            set_sprite_prop(1, 32);
            set_sprite_prop(2, 32);
            set_sprite_prop(3, 32);
            set_sprite_prop(4, 32);
        }
        if (joypad() & J_RIGHT)
        {
            if(plx<256-8&& (!colision_en(plx+8,ply-1))) //que no se salga de la pantalla y que no se choque con algo a la derecha
                ++plx;
            pldir = 1;//direccion derecha
            //devolvemos al personaje a la direccion original
            set_sprite_prop(1, 0);
            set_sprite_prop(2, 0);
            set_sprite_prop(3, 0);
            set_sprite_prop(4, 0);
        }
        if (joypad() & J_UP)
        {
            //--ply;
        }
        if (joypad() & J_DOWN)
        {
            //++ply;
        }

        animarPersonaje();
        //si no se pulsa ni la flecha de izquierda ni la de derecha se reinician los fotogramas y deja de ser animado
        if (!(joypad() & J_LEFT)&&!(joypad() & J_RIGHT)) animFrame=0;
		

        //gravedad
		if (plvy>2) plvy=2; else ++plvy;
		
        //Calcular si hay objeto solido debajo del personaje 
        if (colision_en(plx-7,ply+8)||colision_en(plx+7,ply+8)) {
            ply=(ply>>3)<<3; //que no siga cayendo
            vuelveSaltar = 0;
        }
        else ply+=plvy; //que caiga 


        //al llegar a la puerta que es el sprite 10 pasa de nivel
        if ((ListaNivelesObjetos[nivel_id])[(((ply>>3)-2)<<5)+(((plx>>3)-1)&31)]==10)
        {
            ++nivel_id;
            scroll_bkg(oldplx-88,0);    //reiniciamos la posición de la cámara
            cargarDatos(); 
        }

        //Saltar si boton A presionado
        
		if (joypad() & J_A)
        {
            if (!A_pressed && vuelveSaltar < 2)
            {
                --ply;
                A_pressed=1;
                plvy=-10;

                vuelveSaltar++;
				
                //estos registros representan el primer canal de sonido (representa el salto)
                NR10_REG = 0x16;    //volumen del sonido
                NR11_REG = 0x40;    //longitud del sonido
                NR12_REG = 0x73;    //volumen envolvente
                NR13_REG = 0x00;    //frecuencia baja
                NR14_REG = 0xC3;    //frecuencia alta
            }
        } else A_pressed=0;

        //Lanzar una bala si boton B presionado
        if (joypad() & J_B)
        {
            if (!B_pressed)
            {
                setupBalas();
                ++b;
                B_pressed=1;
				
				NR41_REG = 0x1F;
                NR42_REG = 0xF1;
                NR43_REG = 0x30;
                NR44_REG = 0xC0;
            }
        } else B_pressed=0;
        
        //si se mueve actualizar la pantalla y guardar posicion nueva
		if(oldplx<plx) {scrollX=-1; oldplx=plx;}
        if(oldplx>plx) {scrollX=1; oldplx=plx;}

        if (plx>=80-!pldir && plx<256-80+pldir) //Si esta lejos del borde del fondo mover la camara con el jugador
        {
            moverObjeto(&jugador, 80, ply);
            scroll_bkg(-scrollX,0); //comando desplazar pantalla
			
            camx=plx-80; //actualizar la camara cuando se desplace
        } else
		if (plx>=256-80+pldir)
            moverObjeto(&jugador,plx-256-80+pldir-16,ply);
        else
        {
            moverObjeto(&jugador,plx,ply);
            camx=0; //si esta al principio se reincie la camara
        }

        //moverObjeto(&enemigo[0],enemigo[0].x,enemigo[0].y); 
        
		if(oldplx==plx) {scrollX=0;}

        //moverObjeto(&jugador,plx,ply);
        delay(12);
	}
}