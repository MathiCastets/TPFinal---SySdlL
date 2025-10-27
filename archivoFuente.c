#include <stdio.h>
#include <string.h>
#include <stdlib.h> // parapoder usar strtod, exit, entre otras funciones
#include <ctype.h>
#define NUMESTADOS 15+15 //"Memoria" del scanner
#define NUMCOLS 13+5
#define TAMLEX 32 + 1
#define TAMNOM 20 + 1
/******************Declaraciones Globales*************************/
FILE *in;
typedef enum
{
    INICIO,
    FIN,
    LEER,
    ESCRIBIR,
    ID,
    CONSTANTE,
    PARENIZQUIERDO,
    PARENDERECHO,
    PUNTOYCOMA,
    COMA,
    ASIGNACION,
    SUMA,
    RESTA,
    FDT,
    ERRORLEXICO,

    /** NUEVOS TOKENS**/
    ENTERO,
    REAL,
    CARACTER,
    CONSTANTEREAL,
    CONSTANTECARACTER,
    MIENTRAS,
    FINMIENTRAS,
    SI,
    ENTONCES,
    SINO, //no se si es necesaria, o si basta solo con el si!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    FINSI,
    REPETIR,
    HASTA,
    MAYOR,
    MENOR,
    IGUAL,
    DISTINTO,
    MAYORIGUAL,
    MENORIGUAL
} TOKEN;

typedef enum{ //Tipos de datos que el programador puede definir para luego trabajar con ellos
    TIPOENTERO,
    TIPOREAL,
    TIPOCARACTER,
    TIPONULO //Para manejo de errores
} TIPO;
typedef struct
{
    char identifi[TAMLEX];
    TOKEN t; /* t=0, 1, 2, 3 Palabra Reservada, t=ID=4 Identificador */
    TIPO tipo; //Permite saber si el tipo de dato es ENTERO, REAL o CARACTER
} RegTS;
RegTS TS[1000] = { //Son las palabras reservadas del lenguaje
    {"inicio", INICIO}, 
    {"fin", FIN}, 
    {"leer", LEER}, 
    {"escribir", ESCRIBIR}, 
    //Nuevas palabras reservadas
    {"entero", ENTERO},
    {"real", REAL}, 
    {"caracter", CARACTER},
    {"si", SI},
    {"entonces", ENTONCES},  
    {"sino", SINO}, 
    {"finsi", FINSI}, 
    {"mientras", MIENTRAS}, 
    {"finmientras", FINMIENTRAS},
    {"repetir", REPETIR}, 
    {"hasta", HASTA},  
    {"$", 99}};//CENTINELA - FIN DE TABLA
typedef struct
{
    TOKEN clase;
    char nombre[TAMLEX];
    union            //permite compartir espacio en memoria
    {
        int valor;   //Para constantes enteras
        float valorR; //Para constantes reales
        char valorC; //Para constantes caracteres
    } valor;
    
    TIPO tipo; //Permite saber si el tipo de dato es ENTERO, REAL o CARACTER
} REG_EXPRESION;
char buffer[TAMLEX];
TOKEN tokenActual;
int flagToken = 0;

static unsigned int numEtiqueta = 1; // Variables golbales para manejo de etiquetas

/**********************Prototipos de Funciones************************/
TOKEN scanner();
int columna(int c);
int estadoFinal(int e);
void Objetivo(void);
void Programa(void);
void ListaDeclaraciones(void);
void Declaracion(void);
TIPO TipoDato(void);
void ListaDeIds(TIPO tipo);
void ListaSentencias(void);
void Sentencia(void);
void ListaIdentificadores(void);
void Identificador(REG_EXPRESION *presul);
void ListaExpresiones(void);
void Expresion(REG_EXPRESION *presul);
void Condicion(REG_EXPRESION *presul);    //Nueva funcion para estructuras de control
void OperadorRelacional(char *presul);    //  ''
void Primaria(REG_EXPRESION *presul);
void OperadorAditivo(char *presul);
REG_EXPRESION ProcesarCte(void);
REG_EXPRESION ProcesarId(void);
char *ProcesarOp(void);
void Leer(REG_EXPRESION in);
void Escribir(REG_EXPRESION out);
REG_EXPRESION GenInfijo(REG_EXPRESION e1, char *op, REG_EXPRESION e2);
void Match(TOKEN t);
TOKEN ProximoToken();
void ErrorLexico();
void ErrorSintactico();
void Generar(char *co, char *a, char *b, char *c);
char *Extraer(REG_EXPRESION *preg);
int Buscar(char *id, RegTS *TS, TOKEN *t);
void Colocar(char *id, RegTS *TS, TIPO tipo);
void Chequear(char *s);
void ChequearTemporal(char *s, TIPO tipo);
TIPO ObtenerTipo(char *id);                  //Obtiene el tipo de dato de una variable
TIPO ObtenerTipoExp(REG_EXPRESION *reg);     //Obtiene el tipo de dato de una expresion
char *TipoCadena(TIPO tipo);
void Comenzar(void);
void Terminar(void);
void Asignar(REG_EXPRESION izq, REG_EXPRESION der);
//Rutinas Semanticas para estructuras de control 
void ObtenerEtiqueta(char *etiqueta);
void ImprimirEtiqueta(char *etiqueta);
void Saltar(char *etiqueta);
void ChequearCondicion(REG_EXPRESION cond, char *etiquetaV, char *etiquetaF);

/***************************Programa Principal************************/
int main(int argc, char *argv[])
{
    TOKEN tok;
    char nomArchi[TAMNOM];
    int l;
    /***************************Se abre el Archivo Fuente******************/
    if (argc == 1)
    {
        printf("Debe ingresar el nombre del archivo fuente (en lenguaje Micro) en la linea de comandos \n");
        return -1;
    }
    if (argc != 2)
    {
        printf("Numero incorrecto de argumentos\n");
        return -1;
    }
    strcpy(nomArchi, argv[1]);
    l = strlen(nomArchi);
    if (l > TAMNOM)
    {
        printf("Nombre incorrecto del Archivo Fuente\n");
        return -1;
    }
    if (nomArchi[l - 1] != 'm' || nomArchi[l - 2] != '.')
    {
        printf("Nombre incorrecto del Archivo Fuente\n");
        return -1;
    }
    if ((in = fopen(nomArchi, "r")) == NULL)
    {
        printf("No se pudo abrir archivo fuente\n");
        return -1;
    }
    /*************************Inicio Compilacion***************************/
    Objetivo();
    /**************************Se cierra el Archivo Fuente******************/
    fclose(in);
    return 0;
}
/**********Procedimientos de Analisis Sintactico (PAS) *****************/
void Objetivo(void)
{
    /* <objetivo> -> <programa> FDT #terminar */
    Programa();
    Match(FDT);
    Terminar();
}
void Programa(void)
{
    /* <programa> -> #comenzar INICIO <listaSentencias> FIN */
    Comenzar(); // invocacion a las rutinas semanticas, en la gramatica se coloca con #
    Match(INICIO);
    ListaDeclaraciones(); // Se encarga de inicializar las variables (necesario para establecer sus tipos)
    ListaSentencias();
    Match(FIN);
}

void ListaDeclaraciones(void) //Se encarga de comprobar si el siguiente token es ENTERO, REAL o CARACTER
{
    /* <ListaDeclaraciones> -> <Declaracion> {<Declaracion>} */
    while (ProximoToken() == ENTERO || ProximoToken() == REAL || ProximoToken() == CARACTER)
    {
        Declaracion();
    }
}

void Declaracion(void) //Procesa una linea de declaracion
{
    TIPO tipo = TipoDato();
    ListaDeIds(tipo);
    Match(PUNTOYCOMA);
}

TIPO TipoDato(void) // Funcion para saber que tipo se esta declarando y consumir el TOKEN
{
    TOKEN t = ProximoToken();
    if(t == ENTERO){
        Match(ENTERO);
        return TIPOENTERO;
    }
    else if(t == REAL)
    {
        Match(REAL);
        return TIPOREAL;
    }
    else if(t == CARACTER)
    {
        Match(CARACTER);
        return TIPOCARACTER;
    } else 
    {
        ErrorSintactico();
        return TIPONULO;
    }
    
}

void ListaDeIds(TIPO tipo) //Procesa la lista de variables
{
    /* <listaDeIds> -> ID #colocar_tipo {COMA ID #colocar_tipo} */
    TOKEN t;
    char bufferId[TAMLEX];

    Match(ID);
    strcpy(bufferId, buffer);
    Colocar(bufferId, TS, tipo);
    for( t = ProximoToken(); t == COMA; t = ProximoToken())
    {
        Match(COMA);
        Match(ID);
        strcpy(bufferId, buffer);
        Colocar(bufferId, TS, tipo);
    }
}
void ListaSentencias(void)
{
    /* <listaSentencias> -> <sentencia> {<sentencia>} */
    while (ProximoToken() == ID || ProximoToken() == LEER || ProximoToken() == ESCRIBIR || ProximoToken() == SI || ProximoToken() == MIENTRAS || ProximoToken() == REPETIR)
    {
      Sentencia();
    
    }  // Mientras el proximo token sea alguno de los que inician una sentencia, sigo procesando sentencias
}  
void Sentencia(void)
{
    TOKEN tok = ProximoToken();
    REG_EXPRESION izq, der, cond;
    char etiqueta1[TAMLEX], etiqueta2[TAMLEX], etiqueta3[TAMLEX];
    switch (tok)
    {
    case ID: /* <sentencia> -> ID := <expresion> #asignar ; (rutina semantica)*/
        Identificador(&izq);
        Match(ASIGNACION);
        Expresion(&der);
        Asignar(izq, der);
        Match(PUNTOYCOMA);
        break;
    case LEER: /* <sentencia> -> LEER ( <listaIdentificadores> ) */
        Match(LEER);
        Match(PARENIZQUIERDO);
        ListaIdentificadores();
        Match(PARENDERECHO);
        Match(PUNTOYCOMA);
        break;
    case ESCRIBIR: /* <sentencia> -> ESCRIBIR ( <listaExpresiones> ) */
        Match(ESCRIBIR);
        Match(PARENIZQUIERDO);
        ListaExpresiones();
        Match(PARENDERECHO);
        Match(PUNTOYCOMA);
        break;

    case SI: 
        Match(SI);
        Condicion(&cond);
        Match(ENTONCES); 
        
        ObtenerEtiqueta(etiqueta1); 
        
        ChequearCondicion(cond, "", etiqueta1); // Si es FALSO, SALTA a etiqueta1
        ListaSentencias(); 

       
        if (ProximoToken() == SINO) //Si hay un sino, genero una etiqueta extra
        {
            ObtenerEtiqueta(etiqueta2);
            Saltar(etiqueta2); 
            ImprimirEtiqueta(etiqueta1);
            Match(SINO);
            ListaSentencias(); 
            ImprimirEtiqueta(etiqueta2); 
        } else {
            ImprimirEtiqueta(etiqueta1); 
        }
        
        Match(FINSI);

        break;
    case MIENTRAS: /* <sentencia> -> MIENTRAS <condicion> <listaSentencias> FINMIENTRAS */
        Match (MIENTRAS);
        ObtenerEtiqueta(etiqueta1);
        ImprimirEtiqueta(etiqueta1); 
        Condicion(&cond);
        ObtenerEtiqueta (etiqueta2);
        ChequearCondicion (cond, "", etiqueta2); 
        ListaSentencias();
        Saltar (etiqueta1);
        Match (FINMIENTRAS);
        ImprimirEtiqueta (etiqueta2);
        break;
    case REPETIR: /* <sentencia> -> REPETIR <listaSentencias> HASTA <condicion> */
        Match (REPETIR);
        ObtenerEtiqueta (etiqueta1);
        ImprimirEtiqueta(etiqueta1);
        ListaSentencias();
        Match (HASTA);
        Condicion(&cond);
        // Si la condición es FALSA, salta al inicio (etiqueta1)
        // Se genera un salto condicional, asumiendo que el código intermedio soporta salto si FALSO.
        // Aquí se simula el comportamiento: Chequea la condición. Si es falso, salta al inicio (etiqueta1).
        // Se usa una etiqueta temporal (etiqueta2) para la instrucción siguiente al bucle (salida).
        ObtenerEtiqueta (etiqueta2); //Etiqueta de salida
        ChequearCondicion (cond, etiqueta2, etiqueta1); 
        ImprimirEtiqueta (etiqueta2);
        break;
    default:
     return;
    }
}

void ListaIdentificadores(void)
{
    /* <listaIdentificadores> -> <identificador> #leer_id {COMA <identificador> #leer_id} */
    TOKEN t;
    REG_EXPRESION reg;
    Identificador(&reg);
    Leer(reg);
    for (t = ProximoToken(); t == COMA; t = ProximoToken())
    {
        Match(COMA);
        Identificador(&reg);
        Leer(reg);
    }
}
void Identificador(REG_EXPRESION *presul)
{
    /* <identificador> -> ID #procesar_id */
    Match(ID);
    *presul = ProcesarId();
}
void ListaExpresiones(void)
{
    /* <listaExpresiones> -> <expresion> #escribir_exp {COMA <expresion> #escribir_exp} */
    TOKEN t;
    REG_EXPRESION reg;
    Expresion(&reg);
    Escribir(reg);
    for (t = ProximoToken(); t == COMA; t = ProximoToken())
    {
        Match(COMA);
        Expresion(&reg);
        Escribir(reg);
    }
}
void Expresion(REG_EXPRESION *presul)

{
    /* <expresion> -> <primaria> { <operadorAditivo> <primaria> #gen_infijo } */
    REG_EXPRESION operandoIzq, operandoDer;
    char op[TAMLEX];
    TOKEN t;
    Primaria(&operandoIzq);
    for (t = ProximoToken(); t == SUMA || t == RESTA; t = ProximoToken())
    {
        OperadorAditivo(op);
        Primaria(&operandoDer);
        operandoIzq = GenInfijo(operandoIzq, op, operandoDer);
    }
    *presul = operandoIzq;
}

//Nuevas funciones para estructuras de control
void Condicion (REG_EXPRESION *presul)
{
    /*<condicon> -> <expresion> <operadorRelacional> <expresion>*/
    REG_EXPRESION exp1, exp2, reg;
    char op [TAMLEX];

    Expresion(&exp1);
    OperadorRelacional(op);
    Expresion(&exp2);
    // Generar la comparación y guardar el resultado (0 o 1) en una temporal
    // Usamos GenInfijo de forma extendida para relaciones.
    // Creamos un registro temporal para el resultado booleano (0 o 1)
    reg = GenInfijo(exp1, op, exp2);
    *presul = reg;
}
void OperadorRelacional (char *presul)
{
    TOKEN t = ProximoToken();
    if (t == MAYOR || t == MENOR || t == IGUAL || t == DISTINTO || t== MAYORIGUAL || t == MENORIGUAL)
    {
        Match(t);
        strcpy (presul, ProcesarOp());
    }
    else
    {
        ErrorSintactico();
    }
}

void Primaria(REG_EXPRESION *presul)
{
    TOKEN tok = ProximoToken();
    switch (tok)
    {
    case ID: /* <primaria> -> <identificador> */
        Identificador(presul);
        break;
   case CONSTANTE: 
   case CONSTANTEREAL:
    case CONSTANTECARACTER: /* <primaria> -> CONSTANTE #procesar_cte */
        Match(tok); // Consumimos el token específico
        *presul = ProcesarCte();
        break;
    case PARENIZQUIERDO: /* <primaria> -> PARENIZQUIERDO <expresion> PARENDERECHO */
        Match(PARENIZQUIERDO);
        Expresion(presul);
        Match(PARENDERECHO);
        break;
    default:
        ErrorSintactico(); // Reporta el error
        break; 
    }
}
void OperadorAditivo(char *presul)
{
    /* <operadorAditivo> -> SUMA #procesar_op | RESTA #procesar_op */
    TOKEN t = ProximoToken();
    if (t == SUMA || t == RESTA)
    {
        Match(t);
        strcpy(presul, ProcesarOp());
    }
    else
    {
        ErrorSintactico();
    }
}
/**********************Rutinas Semanticas******************************/
REG_EXPRESION ProcesarCte(void)
{
    /* Convierte cadena que representa numero a numero entero y construye un registro semantico */
    REG_EXPRESION reg;
    reg.clase = CONSTANTE;
    strcpy(reg.nombre, buffer);
    
    if (buffer[0] == '\'')
    {
        reg.valor.valorC = buffer[1]; // El caracter esta en la posicion 1
        reg.tipo = TIPOCARACTER;
    }
    else if (strchr(buffer, '.') != NULL || strchr (buffer, 'e') != NULL || strchr(buffer, 'E') != NULL) // Si tiene punto decimal, es real
    {
        reg.valor.valorR = (float) strtod (buffer, NULL);
        reg.tipo = TIPOREAL;
    }
    else
    {
       sscanf(buffer, "%d", &reg.valor.valor);
        reg.tipo = TIPOENTERO;
    }
    return reg;
}

REG_EXPRESION ProcesarId(void)
{
    /* Declara ID y construye el correspondiente registro semantico */
    REG_EXPRESION reg;
    TOKEN t;

    Chequear(buffer);
    Buscar(buffer, TS, &t);
    TIPO tipo = ObtenerTipo (buffer);
    reg.clase = ID;
    strcpy(reg.nombre, buffer);
    reg.tipo = tipo;
    return reg;
}
char *ProcesarOp(void)
{
    /* Declara OP y construye el correspondiente registro semantico */
    return buffer;
}
void Leer(REG_EXPRESION in)
{
    /* Genera la instruccion para leer */
    TIPO tipo = ObtenerTipo(in.nombre);
    if (tipo == TIPONULO)
    {
        printf("Error Semantico: Variable '%s' no declarada.\n", in.nombre);
        return;
    }
    Generar("Read", in.nombre, TipoCadena(tipo), "");
}
void Escribir(REG_EXPRESION out)
{
    /* Genera la instruccion para escribir */
    TIPO tipo = ObtenerTipoExp(&out);
    Generar("Write", Extraer(&out), TipoCadena(tipo), "");
}

REG_EXPRESION GenInfijo(REG_EXPRESION e1, char *op, REG_EXPRESION e2)
{
    /* Genera la instruccion para una operacion infija y construye un registro semantico con el resultado */
    REG_EXPRESION reg;
    static unsigned int numTemp = 1;
    char cadTemp[TAMLEX] = "Temp&";
    char cadNum[TAMLEX];
    char cadOp[TAMLEX];

    if (op[0] == '+') strcpy(cadOp, "Sumar");
    else if (op[0] == '-') strcpy(cadOp, "Restar");
    else if (strcmp(op, ">") == 0) strcpy(cadOp, "Mayor");
    else if (strcmp(op, "<") == 0) strcpy(cadOp, "Menor");
    else if (strcmp(op, "=") == 0) strcpy(cadOp, "Igual");
    else if (strcmp(op, "<=") == 0) strcpy(cadOp, "MenorIgual");
    else if (strcmp(op, ">=") == 0) strcpy(cadOp, "MayorIgual");
    else if (strcmp(op, "!=") == 0) strcpy(cadOp, "Distinto");
    else {
        // Manejo de error si el operador es desconocido
        strcpy(cadOp, "ErrorOp");
    }

//Generacion de nombre de variable temporal
    sprintf(cadNum, "%d" , numTemp);
    numTemp ++;
    strcat(cadTemp, cadNum);

    // ************ LÓGICA DE TIPO PARA EL RESULTADO ************
    TIPO tipo1 = ObtenerTipoExp(&e1); 
    TIPO tipo2 = ObtenerTipoExp(&e2);
    TIPO tipoResultado = TIPOENTERO;

    // Promoción de tipos:
    if (strcmp(cadOp, "Sumar") == 0 || strcmp(cadOp, "Restar") == 0) {
        // Si alguno es REAL, el resultado es REAL.
        if (tipo1 == TIPOREAL || tipo2 == TIPOREAL) {
            tipoResultado = TIPOREAL;
        } else {
            tipoResultado = TIPOENTERO; // Si ambos son enteros o caracteres, el resultado es entero.
        }
    } 
    else 
    {
        tipoResultado = TIPOENTERO; 
    }
    
    // Chequear (registrar) la variable temporal con el TIPO CORRECTO
    ChequearTemporal(cadTemp, tipoResultado); 
    
    // ************ GENERACIÓN DE CÓDIGO ************
    Generar(cadOp, Extraer(&e1), Extraer(&e2), cadTemp);
    
    // Asignar el tipo y nombre al registro de resultado
    strcpy(reg.nombre, cadTemp);
    reg.tipo = tipoResultado; 
    return reg;
}
/**********************************Funciones Auxiliares**********************************/
void Match(TOKEN t)
{
    if (!(t == ProximoToken()))
        ErrorSintactico();
    flagToken = 0;
}
TOKEN ProximoToken()
{
    if (!flagToken)
    {
        tokenActual = scanner();
        if (tokenActual == ERRORLEXICO)
            ErrorLexico();
        flagToken = 1;
        if (tokenActual == ID)
        {
            Buscar(buffer, TS, &tokenActual);
        }
    }
    return tokenActual;
}
void ErrorLexico()
{
    printf("Error Lexico\n");
    exit(1);
}
void ErrorSintactico()
{
    printf("Error Sintactico\n");
    exit(1);
}
void Generar(char *co, char *a, char *b, char *c)
{
    /* Produce la salida de la instruccion para la MV por stdout */
    printf("%s %s%c%s%c%s\n", co, a, ',', b, ',', c);
}
char *Extraer(REG_EXPRESION *preg)
{
    /* Retorna la cadena del registro semantico */
    return preg->nombre;
}
int Buscar(char *id, RegTS *TS, TOKEN *t)
{
    /* Determina si un identificador esta en la TS */
    int i = 0;
    while (strcmp("$", TS[i].identifi))
    {
        if (!strcmp(id, TS[i].identifi))
        {
            *t = TS[i].t;
            return 1;
        }
        i++;
    }
    return 0;
}
void Colocar(char *id, RegTS *TS, TIPO tipo)
{
    /* Agrega un identificador a la TS */
    //int i = 4; HARDCODEADO, NO ME SIRVE
    int i =0;

    while (strcmp("$", TS[i].identifi))
        i++;
    
        TOKEN t_temp;
    if (Buscar(id, TS, &t_temp))
    {
        printf("Error Semantico: Variable '%s' ya declarada.\n", id);
        return;
    }

    if (i < 999)
    {
        strcpy(TS[i].identifi, id);
        TS[i].t = ID;
        TS[i].tipo = tipo; //Guardamos el tipo en la TS
        Generar("Declara", id, TipoCadena(tipo), "");
        strcpy(TS[++i].identifi, "$");
    }
}
void Chequear(char *s)
{
    /* Si la cadena No esta en la Tabla de Simbolos la agrega,
    y si es el nombre de una variable genera la instruccion */
    TOKEN t;
    if (strncmp(s, "Temp&", 5) != 0) // Si es una variable temporal
    { 
        if (!Buscar(s, TS, &t))
        {
            printf("Error Semantico: Variable '%s' no declarada.\n", s);
            exit(1);
        }
    }
}

void ChequearTemporal(char *s, TIPO tipo)
{
    /* Si la cadena No esta en la Tabla de Simbolos la agrega,
    y si es el nombre de una variable genera la instruccion */
    TOKEN t;
    if (!Buscar(s, TS, &t)) 
    { 
        int i = 0;
        while (strcmp("$", TS[i].identifi))
            i++;
        if (i < 999)
        {
            strcpy(TS[i].identifi, s);
            TS[i].t = ID;
            TS[i].tipo = tipo; //Guardamos el tipo en la TS
            Generar("Declara", s, TipoCadena(tipo), "");
            strcpy(TS[++i].identifi, "$");
        }
    }
}

TIPO ObtenerTipo(char *id)   
{
    /* Retorna el tipo de dato de la variable id */
    int i = 0;
    while (strcmp("$", TS[i].identifi))
    {
        if (!strcmp(id, TS[i].identifi))
        {
            return TS[i].tipo;
        }
        i++;
    }
    return TIPONULO; // Si no se encuentra, retorna TIPONULO
}

TIPO ObtenerTipoExp(REG_EXPRESION *reg)
{ 
    if (reg -> clase == ID || strncmp(reg->nombre, "Temp&", 5) == 0)
    {
        return ObtenerTipo(reg->nombre); 
    }
    else if (reg->clase == CONSTANTE)
    {
        return reg->tipo;
    }
    return TIPONULO;
}

void Comenzar(void)
{
    /* Inicializaciones Semanticas */
}
void Terminar(void)
{
    /* Genera la instruccion para terminar la ejecucion del programa */
    Generar("Detiene", "", "", "");
}
void Asignar(REG_EXPRESION izq, REG_EXPRESION der)
{
    /* Genera la instruccion para la asignacion */
    TIPO tipo_izq = ObtenerTipo(izq.nombre);   
    
    if (tipo_izq == TIPONULO)
    {
        return;
    }

    TIPO tipo_der = ObtenerTipoExp(&der);

    // CASO 1: Tipos identicos
    if (tipo_izq == tipo_der)
    {
        Generar("Almacena", Extraer(&der), izq.nombre, "");
        return;
    }
    
    // CASO 2: Promoción permitida (real = entero)
    if (tipo_izq == TIPOREAL && tipo_der == TIPOENTERO)
    {
        // ¡Permitido! La máquina virtual manejará la conversión.
        Generar("Almacena", Extraer(&der), izq.nombre, "");
        return;
    }

    //CASOS restantes: son errores
    printf("Error Semantico: No se puede asignar un %s a una variable de tipo %s.\n", TipoCadena(tipo_der), TipoCadena(tipo_izq));
}

char *TipoCadena(TIPO tipo)
{
    /* Convierte el tipo de dato a cadena */
    switch (tipo)
    {
    case TIPOENTERO:
        return "Entero";
    case TIPOREAL:
        return "Real";
    case TIPOCARACTER:
        return "Caracter";
    default:
        return "ErrorTipo";
    }
}

void ObtenerEtiqueta(char *etiqueta){ //Obtiene el nombre de la etiqueta
    sprintf(etiqueta, "Etq%d", numEtiqueta++);
}

void ImprimirEtiqueta(char *etiqueta){ // Imprime una etiqueta ya generada
    Generar("Etiqueta", etiqueta, "", "");
}

void Saltar(char *etiqueta) {
   //Función: Genera un salto incondicional ('Salto') a la etiqueta especificada.
    Generar("Salto", etiqueta, "","");
}

void ChequearCondicion(REG_EXPRESION cond, char *etiquetaV, char *etiquetaF) {
    //Función: Genera los saltos condicionales ('SaltoSiFalso'/'SaltoSiVerdadero')
    //basados en el valor booleano (0 o 1) de la temporal 'cond'.

    
    // Si es Falso (0), salta a etiquetaF
    if (strcmp(etiquetaF, "") != 0) {
        Generar("SaltoSiFalso", cond.nombre, etiquetaF, "");
    }
    // Si es Verdadero (1), salta a etiquetaV
    if (strcmp(etiquetaV, "") != 0) {
        Generar("SaltoSiVerdadero", cond.nombre, etiquetaV, "");
    }
}



/**************************Scanner************************************/
TOKEN scanner()
{
    int tabla[NUMESTADOS][NUMCOLS] = {
        {1, 3, 5, 6, 7, 8, 9, 10, 11, 21, 15, 18, 25, 22, 28, 13, 0, 14},
        {1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 16, 4, 4, 4, 4, 4, 4, 4},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 12, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 16, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {17, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 14, 19, 19, 19, 14, 19, 19},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 20, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {24, 24, 24, 24, 24, 24, 24, 24, 24, 23, 24, 24, 24, 24, 24, 24, 24, 24},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {27, 27, 27, 27, 27, 27, 27, 27, 27, 26, 27, 27, 27, 27, 27, 27, 27, 27},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 29, 14, 14, 14, 14, 14, 14, 14, 14},
        {14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14}};
    int car;
    int col;
    int estado = 0;
    int i = 0;
    do
    {
        car = fgetc(in);
        col = columna(car);
        estado = tabla[estado][col];

        if (col != 16)
        {
            //if(estado != 16)
            //{
            buffer[i] = car;
            i++;
            //}
        }
    
    } while (!estadoFinal(estado) && !(estado == 14));

    buffer[i] = '\0';

    switch (estado)
    {
    case 2:
        if (col != 16)
        {
            ungetc(car, in);
            buffer[i - 1] = '\0';
        }
        return ID;
    case 4:
        if (col != 16)
        {
            ungetc(car, in);
            buffer[i - 1] = '\0';
        }
        return CONSTANTE;
    case 5:
        return SUMA;
    case 6:
        return RESTA;
    case 7:
        return PARENIZQUIERDO;
    case 8:
        return PARENDERECHO;
    case 9:
        return COMA;
    case 10:
        return PUNTOYCOMA;
    case 12:
        return ASIGNACION;
    case 13:
        return FDT;
    case 14:
        return ERRORLEXICO;
    case 17:
        if (col != 16)
        {
            ungetc(car, in);
            buffer[i - 1] = '\0';
        }
        return CONSTANTEREAL;
    case 20: // No es necesario ungetc, ' cierra el token
        return CONSTANTECARACTER;
    case 21:
        return IGUAL;
    case 23:
        return MAYORIGUAL;
    case 24:
        if (col != 16)
        {
            ungetc(car, in);
            buffer[i - 1] = '\0';
        }
        return MAYOR;     
    case 26: 
        return MENORIGUAL;
    case 27:
        if(col != 16){
            ungetc(car, in);
            buffer[i - 1] = '\0';
        }
    return MENOR;
    case 29: // FIN_DISTINTO
        return DISTINTO;
    }
    return 0;
}
int estadoFinal(int e)
{
    if (e == 0 || e == 1 || e == 3 || e == 11 || e == 14 || e == 15 || e == 16 || e == 18 || e == 19 || e == 22 || e == 25 || e == 28) //Estados NO finales
        return 0;
    return 1;
}
int columna(int c)
{
    if (isalpha(c))
        return 0;
    if (isdigit(c))
        return 1;
    if (c == '+')
        return 2;
    if (c == '-')
        return 3;
    if (c == '(')
        return 4;
    if (c == ')')
        return 5;
    if (c == ',')
        return 6;
    if (c == ';')
        return 7;
    if (c == ':')
        return 8;
    if (c == '=')
        return 9;
    if(c == '.')
        return 10;
    if(c == '\'')
        return 11;  
    if(c == '<')
        return 12;
    if(c == '>')
        return 13;    
    if(c == '!')
        return 14;    
    if (c == EOF)
        return 15;
    if (isspace(c))
        return 16;
    return 17;
}
/*************Fin Scanner*************/