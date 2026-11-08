package com.example.morsevoz

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.speech.RecognizerIntent
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import java.util.UUID

class MainActivity : AppCompatActivity() {

    // UUID de UART transparente de los módulos BLE tipo HC-05 / HM-10
    private val UART_SERVICE: UUID = UUID.fromString("0000ffe0-0000-1000-8000-00805f9b34fb")
    private val UART_CHAR: UUID = UUID.fromString("0000ffe1-0000-1000-8000-00805f9b34fb")

    private lateinit var tvEstado: TextView
    private lateinit var tvTexto: TextView

    private var gatt: BluetoothGatt? = null
    private var writeChar: BluetoothGattCharacteristic? = null
    private var buscando = false

    private val main = Handler(Looper.getMainLooper())

    private val speechLauncher =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            if (result.resultCode == Activity.RESULT_OK) {
                val datos = result.data?.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS)
                if (!datos.isNullOrEmpty()) tvTexto.text = datos[0]
            }
        }

    private val permisoLauncher =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { res ->
            if (res.values.all { it }) conectar() else toast("Falta el permiso de Bluetooth")
        }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        tvEstado = findViewById(R.id.tvEstado)
        tvTexto = findViewById(R.id.tvTexto)
        findViewById<Button>(R.id.btnConectar).setOnClickListener { pedirPermisoYConectar() }
        findViewById<Button>(R.id.btnHablar).setOnClickListener { hablar() }
        findViewById<Button>(R.id.btnEnviar).setOnClickListener { enviar() }
    }

    private fun pedirPermisoYConectar() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            val permisos = arrayOf(
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.BLUETOOTH_SCAN
            )
            val faltan = permisos.any {
                ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
            }
            if (faltan) { permisoLauncher.launch(permisos); return }
        }
        conectar()
    }

    @SuppressLint("MissingPermission")
    private fun conectar() {
        val adapter = (getSystemService(BLUETOOTH_SERVICE) as BluetoothManager).adapter
        if (adapter == null || !adapter.isEnabled) { toast("Enciende el Bluetooth"); return }

        // 1) Si ya está emparejado, conecta directo
        val emparejado = adapter.bondedDevices.firstOrNull { (it.name ?: "").contains("HC-0", true) }
        if (emparejado != null) {
            tvEstado.text = "Conectando..."
            gatt = emparejado.connectGatt(this, false, gattCallback)
            return
        }

        // 2) Si no, lo busca por BLE
        val scanner = adapter.bluetoothLeScanner
        if (scanner == null) { toast("No hay Bluetooth LE"); return }
        tvEstado.text = "Buscando HC-05..."
        buscando = true
        scanner.startScan(scanCallback)
        main.postDelayed({
            if (buscando) {
                buscando = false
                scanner.stopScan(scanCallback)
                tvEstado.text = "No se encontró el HC-05"
            }
        }, 10000)
    }

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission")
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val nombre = result.device.name ?: result.scanRecord?.deviceName
            if (nombre != null && nombre.contains("HC-0", true) && buscando) {
                buscando = false
                val adapter = (getSystemService(BLUETOOTH_SERVICE) as BluetoothManager).adapter
                adapter.bluetoothLeScanner?.stopScan(this)
                main.post { tvEstado.text = "Conectando..." }
                gatt = result.device.connectGatt(this@MainActivity, false, gattCallback)
            }
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                g.discoverServices()
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                writeChar = null
                main.post { tvEstado.text = "Desconectado" }
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            // Busca la característica de escritura: primero la típica FFE1,
            // si no, la primera que permita escribir.
            var c = g.getService(UART_SERVICE)?.getCharacteristic(UART_CHAR)
            if (c == null) {
                buscar@ for (s in g.services) {
                    for (ch in s.characteristics) {
                        val p = ch.properties
                        if (p and BluetoothGattCharacteristic.PROPERTY_WRITE != 0 ||
                            p and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE != 0) {
                            c = ch; break@buscar
                        }
                    }
                }
            }
            writeChar = c
            main.post {
                tvEstado.text = if (c != null) "Conectado ✅"
                else "Conectado, pero sin canal de datos"
            }
        }
    }

    private fun hablar() {
        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH).apply {
            putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
            putExtra(RecognizerIntent.EXTRA_LANGUAGE, "es-CO")
            putExtra(RecognizerIntent.EXTRA_PROMPT, "Habla ahora")
        }
        try {
            speechLauncher.launch(intent)
        } catch (e: Exception) {
            toast("Este celular no tiene reconocimiento de voz")
        }
    }

    @SuppressLint("MissingPermission")
    private fun enviar() {
        val texto = tvTexto.text.toString()
        val c = writeChar
        val g = gatt
        if (g == null || c == null) { toast("Primero conéctate al HC-05"); return }
        if (texto.isBlank()) { toast("No hay texto para enviar"); return }

        val datos = (texto + "\n").toByteArray()
        val tipo = if (c.properties and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE != 0)
            BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
        else BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT

        // BLE envía de a ~20 bytes: se parte el texto en trozos
        Thread {
            var i = 0
            while (i < datos.size) {
                val fin = minOf(i + 20, datos.size)
                val trozo = datos.copyOfRange(i, fin)
                try {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                        g.writeCharacteristic(c, trozo, tipo)
                    } else {
                        @Suppress("DEPRECATION")
                        run {
                            c.value = trozo
                            c.writeType = tipo
                            g.writeCharacteristic(c)
                        }
                    }
                } catch (_: Exception) {}
                Thread.sleep(40)
                i = fin
            }
            main.post { toast("Enviado") }
        }.start()
    }

    private fun toast(m: String) = Toast.makeText(this, m, Toast.LENGTH_SHORT).show()

    @SuppressLint("MissingPermission")
    override fun onDestroy() {
        super.onDestroy()
        try { gatt?.close() } catch (_: Exception) {}
    }
}
