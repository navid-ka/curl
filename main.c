#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct curl_write_callback {
  char *data;
  size_t size;
} t_callback;

/*
size_t write_callback(void *ptr, size_t size, size_t nmemb, void *data) {
  size_t realsize = size * nmemb;
  t_callback *mem = (t_callback *)data;
  mem->data = realloc(mem->data, mem->size + realsize + 1);
  if (mem->data) {
    memcpy(&(mem->data[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;
  }
  return realsize;
} */ // This only works for text files and not for binary files

/*
void write_to_file(FILE *file, char *data) {
  if (file) {
    fprintf(file, "%s", data);
  }
} */ // This only works for text files and not for binary files

int main(int argc, char **argv) {

  if (argc < 2 || argc > 3) {
    printf("Usage: %s <url> <outfile>\n", argv[0]);
    return 1;
  }
  t_callback chunk;
  chunk.data = malloc(1);
  chunk.size = 0;
  char *url = argv[1];
  FILE *file = fopen(argv[2], "wb");
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    fclose(file);
    return 1;
  }
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      fclose(file);
    }
    // write_to_file(file, chunk.data); Not working for binary files
    curl_easy_cleanup(curl);
  }
  free(chunk.data);
  fclose(file);
  return 0;
}
