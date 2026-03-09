package SpiderRobot;

import javax.swing.*;
import java.awt.*;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;

public class RobotController {

    static PrintWriter writer;

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
        frame.setSize(300, 200);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        JButton forward = new JButton("Avanti");
        JButton left = new JButton("Sinistra");
        JButton right = new JButton("Destra");

        forward.addActionListener(e -> sendCommand("F"));
        left.addActionListener(e -> sendCommand("L"));
        right.addActionListener(e -> sendCommand("R"));

        JPanel panel = new JPanel(new BorderLayout());
        panel.add(forward, BorderLayout.NORTH);

        JPanel bottom = new JPanel();
        bottom.add(left);
        bottom.add(right);
        panel.add(bottom, BorderLayout.CENTER);

        frame.add(panel);
        frame.setVisible(true);
    }

    static void sendCommand(String cmd){
        if(writer != null){
            writer.println(cmd);
            System.out.println("Sent: " + cmd);
        }
    }
}