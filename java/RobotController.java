import javax.swing.*;
import java.awt.*;
import java.io.PrintWriter;
import java.net.Socket;

public class RobotController {

    static PrintWriter writer;
    static boolean isManual = true;

    static JButton forward;
    static JButton left;
    static JButton right;
    static JButton backward;
    static JButton modeToggle;

    public static void main(String[] args) {

        // CONNECT TO RASPBERRY PI
        try {
            Socket socket = new Socket("192.168.1.100", 5000); // CHANGE THIS
            writer = new PrintWriter(socket.getOutputStream(), true);
            System.out.println("Connected via WiFi");
        } catch(Exception e){
            JOptionPane.showMessageDialog(null, "Connection failed!");
            System.exit(1);
        }

        // GUI
        JFrame frame = new JFrame("Robot Controller");
        frame.setSize(300, 300);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        forward = new JButton("Avanti");
        left = new JButton("Sinistra");
        right = new JButton("Destra");
        backward = new JButton("Stop");
        modeToggle = new JButton("Modalità: Manuale");

        forward.addActionListener(e -> sendCommand("F"));
        left.addActionListener(e -> sendCommand("L"));
        right.addActionListener(e -> sendCommand("R"));
        backward.addActionListener(e -> sendCommand("S"));

        modeToggle.addActionListener(e -> toggleMode());

        JPanel panel = new JPanel(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();

        gbc.gridx = 1; gbc.gridy = 0;
        panel.add(forward, gbc);

        gbc.gridx = 0; gbc.gridy = 1;
        panel.add(left, gbc);

        gbc.gridx = 2; gbc.gridy = 1;
        panel.add(right, gbc);

        gbc.gridx = 1; gbc.gridy = 2;
        panel.add(backward, gbc);

        gbc.gridx = 0; gbc.gridy = 3;
        gbc.gridwidth = 3;
        panel.add(modeToggle, gbc);

        frame.add(panel);
        frame.setVisible(true);
    }

    static void sendCommand(String cmd) {
        if (writer != null && isManual) {
            writer.println(cmd);
            System.out.println("Sent: " + cmd);
        }
    }

    static void toggleMode() {
        isManual = !isManual;

        if (isManual) {
            modeToggle.setText("Modalità: Manuale");
            sendRaw("MODE_MANUAL");
        } else {
            modeToggle.setText("Modalità: IA");
            sendRaw("MODE_AI");
        }

        forward.setEnabled(isManual);
        left.setEnabled(isManual);
        right.setEnabled(isManual);
        backward.setEnabled(isManual);
    }

    static void sendRaw(String cmd) {
        if (writer != null) {
            writer.println(cmd);
            System.out.println("Sent: " + cmd);
        }
    }
}