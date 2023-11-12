#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

enum class Mode {
  prefix_computing,
  finding_substr,
};

size_t kmp(const char* input_string, size_t* pref, size_t len, const char* string_to_compare, size_t n, Mode mode,
           size_t cur = 0) {
  for (size_t i = 0; i < n; i++) {
    while (input_string[cur] != string_to_compare[i] && cur != 0) {
      cur = pref[cur - 1];
    }
    if (input_string[cur] == string_to_compare[i]) {
      cur++;
      if (mode == Mode::prefix_computing) {
        if (i == 0) {
          cur--;
        }
        pref[i] = cur;
      } else if (cur == len) {
        return -1;
      }
    }
  }
  return cur;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    fputs("Invalid number of arguments. Usage: <input_file> <input_string>\n", stderr);
    return EXIT_FAILURE;
  }
  FILE* fd = fopen(argv[1], "rb");
  if (fd == nullptr) {
    perror("Could not open file");
    return EXIT_FAILURE;
  }
  constexpr size_t BUF_SIZE = 4096;
  std::array<char, BUF_SIZE> buf{};
  size_t len = strlen(argv[2]);
  char* input_string = argv[2];
  auto* pref = static_cast<size_t*>(malloc(len * sizeof(size_t)));
  if (pref == nullptr) {
    fclose(fd);
    perror("Could not allocate file");
    return EXIT_FAILURE;
  }
  memset(pref, 0, len * sizeof(size_t));

  kmp(input_string, pref, len, input_string, len, Mode::prefix_computing);
  size_t cur = 0;
  do {
    size_t read_n = fread(buf.data(), sizeof(char), buf.size(), fd);
    if (ferror(fd)) {
      perror("Error while reading the file");
      fclose(fd);
      return EXIT_FAILURE;
    }
    cur = kmp(input_string, pref, len, buf.data(), read_n, Mode::finding_substr, cur);
    if (cur == -1) {
      puts("Yes\n");
      fclose(fd);
      free(pref);
      return EXIT_SUCCESS;
    }
  } while (!feof(fd));
  puts("No\n");
  fclose(fd);
  free(pref);
  return EXIT_SUCCESS;
}
