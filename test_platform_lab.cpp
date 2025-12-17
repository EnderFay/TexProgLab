int main(){
  std::cout<< "Кросс-платформенная проверка:\n";

#ifdef _WIN32
  std::cout << "Платформа: Windows\n";
  std::cout << "INVALID_SOCKET = "<< INVALID_SOCKET_TYPE << "\n";
#else
  std::cout << "Платформа Linux/MacOs\n";
  std::cout << "INVALID_SOCKET = "<< INVALID_SOCKET_TYPE << "\n";
#endif

  std::cout << "Размер сокета"<< sizeof(SOCKET_TYPE) << "байт\n";
  return 0;
}
