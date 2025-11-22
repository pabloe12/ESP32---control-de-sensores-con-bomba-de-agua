#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==============================
// CONFIGURACIÓN DE PINES Y SENSORES
// ==============================
#define DHTPIN 15
#define DHTTYPE DHT11
#define MQ2PIN 34

DHT dht(DHTPIN, DHTTYPE);

// Pantalla LCD I2C 16x2 (dirección 0x27, puede variar)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==============================
// VARIABLES GLOBALES
// ==============================
unsigned long ultimoTiempoLectura = 0;
const unsigned long INTERVALO_LECTURA = 3000; // Cada 3 segundos

float humedadAnterior = 0;
float temperaturaAnterior = 0;

// ==============================
// CONFIGURACIÓN INICIAL
// ==============================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== MONITOREO DE INVERNADERO ===");

  // Inicializar DHT11
  dht.begin();
  if (!verificarDHT()) Serial.println("❌ ERROR: Sensor DHT11 no responde");
  else Serial.println("✅ Sensor DHT11 listo");

  // Configurar MQ2 ADC
  analogReadResolution(12);         // 12 bits = 0-4095
  analogSetAttenuation(ADC_11db);  // rango ~0-3.6V
  pinMode(MQ2PIN, INPUT);

  // Inicializar LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistema listo");
}

// ==============================
// FUNCIONES DE LECTURA DE SENSORES
// ==============================
bool verificarDHT() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  return !isnan(temp) && !isnan(hum);
}

// ==============================
// BUCLE PRINCIPAL
// ==============================
void loop() {
  unsigned long tiempoActual = millis();
  if (tiempoActual - ultimoTiempoLectura >= INTERVALO_LECTURA) {
    ultimoTiempoLectura = tiempoActual;

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    int gasRaw = analogRead(MQ2PIN);
    int gas = map(gasRaw, 0, 4095, 0, 1023); // Normalización 0–1023

    bool errorSensores = false;
    if (isnan(temp) || isnan(hum)) {
      Serial.println("⚠ Error al leer el DHT11");
      errorSensores = true;
    }
    if (gasRaw == 0) {
      Serial.println("⚠ Error al leer el sensor MQ2");
      errorSensores = true;
    }

    if (!errorSensores) {
      mostrarDatos(temp, hum, gas);
      mostrarLCD(temp, hum, gas);
    } else {
      Serial.println("❌ Algunos sensores no están respondiendo");
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ERROR SENSORES");
    }
  }
}

// ==============================
// FUNCIONES DE SALIDA
// ==============================
void mostrarDatos(float temp, float hum, int gas) {
  Serial.println("\n--- LECTURA DE SENSORES ---");
  Serial.print("🌡 Temperatura: "); Serial.print(temp); Serial.println(" °C");
  Serial.print("💧 Humedad: "); Serial.print(hum); Serial.println(" %");
  Serial.print("🔥 Nivel de gas: "); Serial.println(gas);

  String riesgo = calcularRiesgo(temp, hum, gas);
  Serial.println("📊 Nivel de riesgo: " + riesgo);
}

// ==============================
// MOSTRAR EN LCD
// ==============================
void mostrarLCD(float temp, float hum, int gas) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(temp,1);
  lcd.print("C H:");
  lcd.print(hum,0);
  lcd.print("%");

  lcd.setCursor(0,1);
  String riesgo = calcularRiesgo(temp, hum, gas);
  lcd.print("Gas:");
  lcd.print(gas);
  lcd.setCursor(10,1);
  lcd.print(riesgo[0]); // Muestra el primer símbolo para alertar visual
}

// ==============================
// FUNCIÓN PARA CLASIFICAR EL RIESGO
// ==============================
String calcularRiesgo(float temp, float hum, int gas) {
  if (temp > 45 && gas > 900 && hum < 25) return "ALTO";
  if ((temp > 40 && gas > 700) || (gas > 800 && hum < 30)) return "MEDIO";
  if (temp < 35 && gas < 400 && hum > 30) return "BAJO";
  return "MOD";
}
