#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "timer.h"

//variáveis globais
int N; //número de elementos por bloco
int blocos; //número de blocos
int contador = 0; //conta quantos blocos já foram consumidos pelas threads
int* estado; //1 se o bloco não tiver sido consumido, 0 se tiver sido
int* buf; //buffer de tamanho N*10
pthread_cond_t *cond; //vetor com variáveis de condição que indicam se bloco foi consumido ou não
pthread_mutex_t cont; //variável de exclusão mútua usada apenas para conferir o contador
pthread_mutex_t impr; //variável de exclusão mútua usada para garantir uma impressão de bloco por vez
pthread_mutex_t *mutex; //vetor com os mutex que permitem a entrada em cada trecho do buffer

//função executada pelo produtor
void* produtor(void* arg){
  for(int i=0 ; i<blocos ; i++){ //para cada bloco
    int entrada = (i%10)*N;  //verifica em que ponto do buffer a thread irá entrar
    pthread_mutex_lock(&mutex[entrada/N]); //verifica se tem thread consumidora naquele bloco
    if(estado[entrada/N]==1){    //se bloco ainda não tiver sido consumido, espera
      pthread_cond_wait(&cond[entrada/N],&mutex[entrada/N]);}
    for(int j=0 ; j<N ; j++){    //para cada elemento do bloco
      scanf("%d",&buf[entrada+j]); //põe os valores do arquivo no buffer
    }
    estado[entrada/N]=1;   //atualiza estado do bloco
    pthread_cond_signal(&cond[entrada/N]);   //sinaliza a thread consumidora
    pthread_mutex_unlock(&mutex[entrada/N]); //libera entrada no bloco
  }
  pthread_exit(NULL);
}

//função que lê um bloco e copia os valores para um vetor
int* le(int entrada){
  int* vetor;
  vetor = (int*) malloc(sizeof(int)*N);
  if(vetor==NULL) { fprintf(stderr,"ERRO--malloc()\n"); exit(-1); }
  for(int i=0; i<N ;i++){
    vetor[i] = buf[entrada+i];
  }
  return vetor;
}

//função que organiza vetor
int sort(int** ptVetor){
  int* vetor = *ptVetor;
  int temp;
  for(int i=0 ; i<N-1 ; i++){
    for(int j=i+1 ; j<N ; j++){
      if(vetor[j]<vetor[i]){
        temp=vetor[i];
        vetor[i]=vetor[j];
        vetor[j]=temp;
      }
    }
  }
  return 0;
}

//função que imprime os blocos ordenados
int imprime(int* vetor){
  pthread_mutex_lock(&impr);
  for(int i=0 ; i<N ; i++){
    printf("%d ",vetor[i]);
  }
  printf("\n");
  pthread_mutex_unlock(&impr);
  return 0 ;
}

//função executada pelos consumidores
void* consumidor(void* arg){
  int entrada;
  pthread_mutex_lock(&cont); //lock para que possa verificar a variável "contador"
  while(contador<blocos){    //enquanto houverem blocos a serem consumidos
    entrada = (contador%10)*N;
    contador++;
    pthread_mutex_unlock(&cont);   //já parou de lidar com contador
    pthread_mutex_lock(&mutex[entrada/N]);
    while(estado[entrada/N]==0){   //se buffer estiver vazio, espera. No produtor pude usar if(), mas aqui tem que ser while()
      pthread_cond_wait(&cond[entrada/N],&mutex[entrada/N]);}
    int* vetor = le(entrada);    //le os valores do buffer. Fiz um vetor novo para já liberar o buffer para o produtor
    estado[entrada/N]=0;        //atualiza estado
    pthread_cond_broadcast(&cond[entrada/N]);    //não sei se isso é muito eficiente, pois só quero atingir a thread produtora
    pthread_mutex_unlock(&mutex[entrada/N]);
    sort(&vetor);           //ordena o bloco
    imprime(vetor);
    free(vetor);            //libera memória
  }
  pthread_mutex_unlock(&cont); //o unlock é feito aqui se thread não entrar no while
  pthread_exit(NULL);
}

//fluxo principal
int main(int argc, char *argv[]) {

  double inicio, fim;
  GET_TIME(inicio);

  //recebe argumentos de entrada
  if(argc<3){
    printf("Formato esperado:  %s <numúmero de consumidores> <tam do bloco>\n",argv[0]);
    return 1;
  }
  int C = atoi(argv[1]); //número de consumidores
  N = atoi(argv[2]); //tamanho do bloco

  //lê a quantidade de números no arquivo
  scanf("%d", &blocos); //coloca temporariamente na variável "blocos"
  blocos = blocos/N; //atribui o  valor correto a "blocos"


  //inicializa vetor estado
  estado = (int*) malloc(sizeof(int)*blocos);
  if(estado==NULL){
    fprintf(stderr,"ERRO--malloc()\n");
    return 2;
  }
  for(int i=0 ; i<blocos ; i++) estado[i]=0;

  //inicializa buffer
  buf = (int*) malloc(sizeof(int)*N*10);
  if(buf==NULL){
    fprintf(stderr,"ERRO--malloc()\n");
    return 2;
  }

  //aloca memória para o vetor de variáveis de condição
  cond = (pthread_cond_t*) malloc(sizeof(pthread_mutex_t)*blocos);
  if(cond==NULL) {
    fprintf(stderr,"ERRO--malloc()\n");
    return 2;
  }

  //aloca memória para o vetor de variáveis mutex
  mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t)*blocos);
  if(mutex==NULL) {
    fprintf(stderr,"ERRO--malloc()\n");
    return 2;
  }

  //cria threads
  pthread_t tid[C+1];
  if(pthread_create(&tid[0],NULL,produtor,NULL)){ //cria produtor
    fprintf(stderr,"ERRO--pthread_create() \n");
    return 3;
  }
  for(int i=1 ; i<C+1 ; i++){               //cria consumidores
    if(pthread_create(&tid[i],NULL,consumidor,NULL)){
      fprintf(stderr,"ERRO--pthread_create() \n");
      return 3;
    }
  }

  //espera finalização das threads
  for (int i=0 ; i<C+1 ; i++){
    if(pthread_join(tid[i],NULL)){
      fprintf(stderr,"ERRO--pthread_join() \n");
        return 4;
    }
  }

  //libera memória
  free(buf);
  for(int i=0 ; i<blocos ; i++)
    pthread_mutex_destroy(&mutex[i]);
  free(mutex);
  pthread_mutex_destroy(&cont);

  GET_TIME(fim);
  printf("%f\n", fim-inicio);

  return 0;
}
