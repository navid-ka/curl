#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

typedef struct data {
  char *url;
  char *file;
} t_data;

void print_download_speed(double bytes, double time) {
    double speed = bytes / time;
    if (speed < 1024) {
        printf("\r\t\t\tSpeed: %.2f B/s", speed);
    } else if (speed < 1024 * 1024) {
        printf("\r\t\t\tSpeed: %.2f KB/s", speed / 1024);
    } else {
        printf("\r\t\t\tSpeed: %.2f MB/s", speed / (1024 * 1024));
    }
    fflush(stdout);
}

int progress_callback(void *p,
                      curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal __attribute__((unused)), 
                      curl_off_t ulnow __attribute__((unused)))
{
    time_t elapsed_time = time(NULL) - *((time_t *)p);
    if (dlnow > 0 && dltotal > 0 && elapsed_time > 0) {
        double download_speed = dlnow / elapsed_time;
        printf("\rThread %lu: \t", pthread_self());
        print_download_speed(download_speed, 1.0);
    }
    return 0;
}

void *downloader(void *data)
{
  t_data *fdata = (t_data *)data;
  char *url = fdata->url;
  FILE *file = fopen(fdata->file, "wb");
  if (!file) {
        fprintf(stderr, "fopen() failed\n");
        return NULL;
    }
  CURL *curl;
  CURLcode res;
  curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "curl_easy_init() failed\n");
    return NULL;
  }
  if (curl) {
    time_t start_time = time(NULL);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &start_time);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        fclose(file);
    }

    curl_easy_cleanup(curl);
  }
  fclose(file);
  return NULL;
}


int main(int argc, char **argv) {

  if (argc < 2 || argc > 4) {
    printf("Usage: %s <url> <outfile> <concurrent files> \n", argv[0]);
    return 1;
  }
  t_data data;
  size_t num_threads = atoi(argv[3]);
  if (!num_threads) {
    fprintf(stderr, "Invalid number of threads\n");
    return 1;
  }
  pthread_t th[num_threads];

  data.url = argv[1];
  data.file = argv[2];
  for (size_t i = 0; i < num_threads; i++) {
    pthread_create(&th[i], NULL, downloader, &data);
  }
  printf("Downloading %s to %s using %ld threads\n", data.url, data.file,
         num_threads);
  for (size_t i = 0; i < num_threads; i++) {
    pthread_join(th[i], NULL);
  }
  return 0;
}
