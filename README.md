# ESP32 DS18B20 temperature

### Board

![Board NodeMCU 32S](images/ESP32_NodeMCU_32S.png "NodeMCU-32S")

### Libraries

- [One Wire](https://github.com/PaulStoffregen/OneWire "One Wire library") library by Paul Stoffregen

  ![Library - OneWire.png](images/Library%20-%20OneWire.png "OneWire")

- [Dallas Temperature](https://github.com/milesburton/Arduino-Temperature-Control-Library "Dallas Temperature")

  ![Dallas Temperature](images/Library%20-%20Dallas%20Temperature.png "Dallas Temperature")

- Add libraries from Sketch -> Include Library -> Add .ZIP Library
  ![Add library from zip file](images/add_library_from_zip_file.png "Add library from zip file")
    + [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer/archive/refs/heads/master.zip "ESPAsyncWebServer") Zip file (.zip folder)
    + [AsyncTCP](https://github.com/me-no-dev/AsyncTCP/archive/refs/heads/master.zip "AsyncTCP") (.zip folder)

### Seven Segment LED Display

- Using TM1637 module to display temperature celsius.
  Choose <a href="https://github.com/AKJ7/TM1637" target="_blank">Library - TM1637 Driver</a> for ESP32
  ![TM1637 Driver](images/Library%20-%20TM1637%20Driver.png "TM1637 Driver")

### SRD-05VDC-SL-C Relay

- Ref: <a href="https://diyi0t.com/relay-tutorial-for-arduino-and-esp8266/" target="_blank">SRD-05VDC-SL-C</a>
