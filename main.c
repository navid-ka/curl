#include <curl/curl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

typedef struct data {
  char *url;
  char *file;
  long start;
  long end;
} t_data;


curl_off_t total_downloaded = 0;
curl_off_t last_dlnow = 0;
time_t start_time;

void print_progress_bar(double progress) {
    int barWidth = 40;

    printf("\r[");
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %.2f%%", progress * 100.0);
}

void print_download_speed(double bytes, double time) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    double speed = bytes / time;
    char speed_str[50];
    if (speed < 1024) {
        sprintf(speed_str, "Speed: %.2f B/s", speed);
    } else if (speed < 1024 * 1024) {
        sprintf(speed_str, "Speed: %.2f KB/s", speed / 1024);
    } else {
        sprintf(speed_str, "Speed: %.2f MB/s", speed / (1024 * 1024));
    }

    int padding = w.ws_col - strlen(speed_str) - 1;
    printf("\r%*s%s", padding, "", speed_str);
    fflush(stdout); 
}

int progress_callback(__attribute__((unused)) void *p, curl_off_t dltotal, curl_off_t dlnow,
                      curl_off_t ultotal __attribute__((unused)), 
                      curl_off_t ulnow __attribute__((unused)))
{
    total_downloaded += dlnow - last_dlnow;
    last_dlnow = dlnow;

    time_t elapsed_time = time(NULL) - start_time;
    if (total_downloaded > 0 && dltotal > 0 && elapsed_time > 0) {
        double download_speed = total_downloaded / elapsed_time;
        double progress = (double)total_downloaded / (double)dltotal;
        printf("\r");
        print_download_speed(download_speed, 1.0);
        print_progress_bar(progress);
        fflush(stdout);
    }
    return 0;
}

long get_file_size(const char *url) {
    CURL *curl;
    CURLcode res;
    curl_off_t filesize = 0;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
            curl_easy_cleanup(curl);
            return -1;
        } else {
            curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &filesize);
        }

        curl_easy_cleanup(curl);
    }

    return (long)filesize;
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

    // Establecer el rango de bytes para descargar
    char range[50];
    sprintf(range, "%ld-%ld", fdata->start, fdata->end);
    curl_easy_setopt(curl, CURLOPT_RANGE, range);

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
  size_t num_threads = atoi(argv[3]);
  if (!num_threads) {
    fprintf(stderr, "Invalid number of threads\n");
    return 1;
  }
  start_time = time(NULL);
  pthread_t th[num_threads];
  t_data data[num_threads];
  long file_size = get_file_size(argv[1]);
  printf("File size: %ld\n", file_size);
  for (size_t i = 0; i < num_threads; i++) {
    data[i].url = argv[1];
    data[i].file = argv[2];
    data[i].start = i * (file_size / num_threads);
    data[i].end = (i + 1) * (file_size / num_threads) - 1;
    pthread_create(&th[i], NULL, downloader, &data[i]);
  }
  printf("Downloading %s to %s using %ld threads\n", argv[1], argv[2],
         num_threads);
  for (size_t i = 0; i < num_threads; i++) {
    pthread_join(th[i], NULL);
  }
  printf("\nDownload complete\n");
  return 0;
}