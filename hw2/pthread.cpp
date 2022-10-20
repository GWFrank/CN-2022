#include<queue>
#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>

#define THREAD_NUM 10
void *do_something(void *ptr);

pthread_t tid[THREAD_NUM];

int main(int argc, char *argv[]){
	// for(int i = 0 ; i < THREAD_NUM; i++){
    //     int *input = (int*)malloc(sizeof(int));
    //     *input = i * 100;
	// 	// create a new thread.
	// 	// pthread_create (ptr to a pthread_id, NULL, function to run, pointer to  the parameter)
	// 	pthread_create(&tid[i], NULL, &do_something, (void*)input);
	// }

    std::queue<int> frame_buffer;
    pthread_create(&tid[0], NULL, &do_something, &frame_buffer);
    pthread_join(tid[0], NULL);
    printf("queue content: ");
    while (frame_buffer.size() > 0) {
        printf("%d ", frame_buffer.front());
        frame_buffer.pop();
    }
    printf("\n");
	// for(int i = 0 ; i < THREAD_NUM; i++){
	// 	// wait a thread to be finished
	// 	// pthread_join(ptr to a pthread_id, NULL)
	// 	pthread_join(tid[i], NULL);
    //     printf("[Index %d] join pthread id %ld\n", i, tid[i]);
	// }
    // pthread_exit(NULL);
    return 0;
}

void *do_something(void *arg){
	// pthread_t id = pthread_self();
    // int input = *(int*)arg;
    // for(int i = 0; i < THREAD_NUM; i++){
    //     if(pthread_equal(id, tid[i])){
    //         printf("[Index %d] pthread id %ld with input value %d\n", i, tid[i], input);
    //         break;
    //     }
    // }
    std::queue<int>* frame_buffer_p = static_cast<std::queue<int>*>(arg);
    frame_buffer_p->push(4);
    frame_buffer_p->push(5);
    frame_buffer_p->push(6);
    pthread_exit((void*)NULL);
}
