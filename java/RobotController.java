mport javax.swing.*;

import java.awt.*;

import java.io.PrintWriter;

import java.net.Socket;


public class RobotController {


    static PrintWriter writer;

    static boolean isManual = true; // Variabile per tenere traccia della modalità


    public static void main(String[] args) {


        // Connetti al Raspberry su WiFi

        try {

            Socket socket = new Socket("192.168.1.123", 5000); // IP Raspberry

            writer = new PrintWriter(socket.getOutputStream(), true);

            System.out.println("Connected via WiFi");

        } catch(Exception e){

            e.printStackTrace();

        }


        // GUI

        JFrame frame = new JFrame("Robot Controller");

        frame.setSize(300, 300);

        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);


        // Creazione dei bottoni

        JButton forward = new JButton("Avanti");

        JButton left = new JButton("Sinistra");

        JButton right = new JButton("Destra");

        JButton backward = new JButton("Indietro");

        JButton modeToggle = new JButton("Modalità: Manuale");


        // Aggiunta degli ascoltatori agli eventi dei bottoni

        forward.addActionListener(e -> sendCommand("F"));

        left.addActionListener(e -> sendCommand("L"));

        right.addActionListener(e -> sendCommand("R"));

        backward.addActionListener(e -> sendCommand("B"));

        

        // Cambia la modalità tra manuale e IA

        modeToggle.addActionListener(e -> toggleMode(modeToggle));


        // Creazione del layout

        JPanel panel = new JPanel(new GridBagLayout());

        GridBagConstraints gbc = new GridBagConstraints();

        

        // Posiziona il tasto "Avanti" al centro (prima riga)

        gbc.gridx = 1;

        gbc.gridy = 0;

        gbc.gridwidth = 1;

        gbc.anchor = GridBagConstraints.CENTER;

        panel.add(forward, gbc);


        // Posiziona i bottoni "Sinistra" e "Destra" ai lati (seconda riga)

        gbc.gridx = 0;

        gbc.gridy = 1;

        gbc.anchor = GridBagConstraints.CENTER;

        panel.add(left, gbc);


        gbc.gridx = 2;

        gbc.gridy = 1;

        gbc.anchor = GridBagConstraints.CENTER;

        panel.add(right, gbc);


        // Posiziona il tasto "Indietro" in basso (terza riga)

        gbc.gridx = 1;

        gbc.gridy = 2;

        gbc.anchor = GridBagConstraints.CENTER;

        panel.add(backward, gbc);


        // Posiziona il tasto per cambiare modalità in basso a sinistra

        gbc.gridx = 0;

        gbc.gridy = 3;

        gbc.gridwidth = 3;

        gbc.anchor = GridBagConstraints.CENTER;

        panel.add(modeToggle, gbc);


        // Aggiungi il pannello alla finestra

        frame.add(panel);

        frame.setVisible(true);

    }


    // Metodo per inviare il comando al Raspberry Pi

    static void sendCommand(String cmd) {

        if (writer != null) {

            writer.println(cmd);

            System.out.println("Sent: " + cmd);

        }

    }


    // Metodo per alternare tra modalità manuale e IA

    static void toggleMode(JButton modeButton) {

        isManual = !isManual;

        if (isManual) {

            modeButton.setText("Modalità: Manuale");

        } else {

            modeButton.setText("Modalità: IA");

        }

        System.out.println(isManual ? "Modalità manuale attivata" : "Modalità IA attivata");

    }