package com.example.projetomestrado

import android.Manifest
import android.content.pm.PackageManager
import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.text.SimpleDateFormat
import java.util.*

class MainActivity : AppCompatActivity() {

    private lateinit var textView: TextView
    private lateinit var sendButton: Button
    private lateinit var readCsvButton: Button
    private lateinit var listEventsButton: Button

    private val MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE = 1
    private var eventsInRangeResult: String = ""

    // Adicionar um padrão de formatação para a classe
    companion object {
        val dateTimeFormat = SimpleDateFormat("h:mm:ss a", Locale.getDefault())
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        textView = findViewById(R.id.sample_text)
        sendButton = findViewById(R.id.send_button)
        readCsvButton = findViewById(R.id.read_csv_button)
        listEventsButton = findViewById(R.id.list_events_button)

        sendButton.setOnClickListener {
            val response = sendMessageToServer("Hello World")
            textView.text = response
        }

        readCsvButton.setOnClickListener {
            // Substitua "/sdcard/Download/lista.csv" pelo caminho real do seu arquivo CSV
            val csvContent = readCsvFile("/sdcard/Download/lista.csv")

            // Exibir o conteúdo do CSV no TextView
            textView.text = csvContent
        }

        listEventsButton.setOnClickListener {
            // Obter os valores dos campos de entrada
            val startDateEditText = findViewById<EditText>(R.id.start_date_edittext)
            val endDateEditText = findViewById<EditText>(R.id.end_date_edittext)

            val startDate = startDateEditText.text.toString()
            val endDate = endDateEditText.text.toString()

            // Chamar a função JNI e armazenar o resultado na variável global
            eventsInRangeResult = listEventsInRangeJNI(startDate, endDate)

            // Exibir o resultado na TextView
            textView.text = eventsInRangeResult

        }

        // Solicitar permissões em tempo de execução, se necessário
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE), MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE)
        }
    }

    // Função para ler o CSV no código nativo
    external fun readCsvFile(filePath: String): String

    // Funções JNI existentes (Hello World e SendMessageToServer)
    external fun stringFromJNI(): String
    external fun sendMessageToServer(message: String): String

    // Função JNI para listar eventos no intervalo de datas
    external fun listEventsInRangeJNI(startDate: String, endDate: String): String

    init {
        System.loadLibrary("projetomestrado")
    }
}
