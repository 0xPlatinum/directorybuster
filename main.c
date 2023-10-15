#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <curl/curl.h>
#include <string.h>
typedef struct {
	char * line;
	char * domain;
} info;
void *test_line(void *);
void addTrailingSlash(char *str) {
    if (str == NULL)
        return;  // Handle NULL pointer

    size_t len = strlen(str);
    if (len > 0 && str[len - 1] != '/') {
        str[len] = '/';
        str[len + 1] = '\0';
    }
}
int main(int argc, char *argv[])
{

    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    if (argc != 3) {
      fprintf(stderr,"usage: %s <domain> <wordlist>\n", argv[0]);
      return 1;
  	}
    char * domain=argv[1];
    fp = fopen(argv[2], "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    int file_length=0;
    while ((read = getline(&line, &len, fp)) != -1) {
    	file_length++;
    }
    fseek(fp, 0, SEEK_SET);
    int num_threads = file_length; 
    pthread_t tids[num_threads];
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    len=0;
    line=NULL;
    int counter=0;
    printf("beginning thread creation...\n");
    while ((read = getline(&line, &len, fp)) != -1) {
    	if (line[read - 1] == '\n')
        line[read - 1] = '\0';
    	info *inst = (info *)(malloc(sizeof(info)));
    	 char *line_copy = strdup(line);
    	 char *domain_copy = strdup(domain);
    	 inst->line = line_copy;
    	 inst->domain=domain_copy;
    	 pthread_create(&tids[counter], &attr, test_line, inst);
    	 counter++;
    }
    printf("Thread creation finished, %d threads. waiting on threads to finish.\n", counter);
    for (int i = 0; i < counter; i++) {
        pthread_join(tids[i], NULL);
    }
    printf("Threads finished.\n");
    fclose(fp);
    if (line)
        free(line);
    exit(EXIT_SUCCESS);
    

}
size_t DiscardCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    // Discard the received data
    return size * nmemb;
}
void *test_line(void *param) {
	CURL *curl;
    CURLcode res;
    long http_code;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
	info *stuff = (info *) param;
	char *domain = stuff->domain;
	size_t domain_len = strlen(domain) + 1;  // +2 for '/' and null terminator
    char *modified_domain = (char *)malloc(domain_len);
	addTrailingSlash(modified_domain);
	char *line = stuff->line;
	size_t line_len = strlen(line);

    // Allocate memory for the query
    size_t query_len = domain_len + line_len + 1;  // +1 for null terminator
    char *query = (char *)malloc(query_len);
    addTrailingSlash(domain);
    snprintf(query, query_len, "%s%s", domain, line);;
	// printf("Line is %s\nQuery is %s\n",line, query);
	if (curl) {
        // Set the URL to fetch
        curl_easy_setopt(curl, CURLOPT_URL, query);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DiscardCallback);
        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            fprintf(stderr, "", curl_easy_strerror(res));
        } else {
            // Retrieve HTTP response code
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }

    // Cleanup curl global state
    curl_global_cleanup();
    // printf("%d\n",http_code);
    if (http_code == 200) {
    	printf("Successful Directory Found %s\n", query);
    }
    free(stuff->line);
    free(modified_domain);
    free(query);
    pthread_exit(NULL);
}