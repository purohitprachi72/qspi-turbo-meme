#include <SPI.h>
#include <InternalFileSystem.h>
#include <SparkFun_KX13X.h>

using namespace Adafruit_LittleFS_Namespace;

#define FILENAME "/test1.bin"

SparkFun_KX134_SPI kxAccel;
rawOutputData myData;
byte chipSelect = 1;
const int N = 30000;

int16_t accelData[N][3];

void setup() {

  pinMode(chipSelect, OUTPUT);
  digitalWrite(chipSelect, HIGH);

  SPI.begin();
  Serial.begin(115200);
  while (!Serial) delay(50);

  if (!kxAccel.begin(chipSelect)) {
    Serial.println("Could not communicate with the KX13X. Freezing.");
    while (1)
      ;
  }

  Serial.println("Accelerometer Ready.");
  if (kxAccel.softwareReset())
    Serial.println("Accelerometer Reset.");

  delay(5);

  kxAccel.enableAccel(false);
  kxAccel.setRange(SFE_KX134_RANGE64G);
  kxAccel.setOutputDataRate(13);  // 6.4kHz
  kxAccel.enableDataEngine();
  kxAccel.enableAccel();

  Serial.println("Starting sampling...");
  Serial.flush();

  for (int i = 0; i < N; i++) {
    if (kxAccel.dataReady()) {
      kxAccel.getRawAccelData(&myData);
      accelData[i][0] = myData.xData;
      accelData[i][1] = myData.yData;
      accelData[i][2] = myData.zData;
    }
    delayMicroseconds(166);
  }

  Serial.println("Sampling done.");
  Serial.println("Starting InternalFS...");
  Serial.flush();

  // Initialize the file system
  if (!InternalFS.begin()) {
    Serial.println("Failed to initialize InternalFS!");
    return;
  }

  // Remove the file if it exists
  if (InternalFS.exists(FILENAME)) {
    InternalFS.remove(FILENAME);
    Serial.println("Removed existing file.");
  }

  // Open file with proper flags - FILE_O_WRITE | FILE_O_CREATE
  File file = InternalFS.open(FILENAME, FILE_O_WRITE);

  if (!file) {
    Serial.println("Failed to open file for writing!");
    return;
  }

  // Write data in smaller chunks to avoid buffer limitations
  const int CHUNK_SIZE = 4096;  // 4KB chunks
  size_t totalBytes = sizeof(accelData);
  size_t bytesWritten = 0;
  const uint8_t* dataPtr = (const uint8_t*)accelData;

  Serial.print("Writing data in chunks. Total size: ");
  Serial.println(totalBytes);

  while (bytesWritten < totalBytes) {
    size_t chunkSize = min(CHUNK_SIZE, totalBytes - bytesWritten);
    size_t written = file.write(dataPtr + bytesWritten, chunkSize);

    if (written == 0) {
      Serial.println("Write error occurred!");
      break;
    }

    bytesWritten += written;

    // Print progress
    if (bytesWritten % (CHUNK_SIZE * 4) == 0 || bytesWritten == totalBytes) {
      Serial.print("Written: ");
      Serial.print(bytesWritten);
      Serial.print(" of ");
      Serial.println(totalBytes);
    }
  }

  file.close();

  if (bytesWritten == totalBytes) {
    Serial.println("Data written successfully!");
  } else {
    Serial.print("Partial write: ");
    Serial.print(bytesWritten);
    Serial.print(" of ");
    Serial.println(totalBytes);
  }

  Serial.println("Done");
  Serial.flush();
}

void loop() {
  delay(2000);
}