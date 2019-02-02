package kuresaru.badapple;

import javax.imageio.ImageIO;
import java.awt.image.BufferedImage;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;

public class DataGen {

    public static FileOutputStream outputStream;

    public static void main(String[] args) throws Exception {
        File[] imgs = new File("C:\\Users\\Kuresaru\\Desktop\\VGA_BadApple\\img").listFiles();
        //test1(parseImg(imgs[88]));
        outputStream = new FileOutputStream(new File("badapple.bin"));
        for (int i = 0; i < imgs.length; i++) {
            File img = imgs[i];
            outputStream.write(parseImg(img));
            outputStream.flush();
            System.out.println(String.format("生成中 %.2f%%", (++i * 100.0f) / imgs.length));
        }
        outputStream.close();
    }

    /**
     * 把一帧图像转换成32个字节的数据
     *
     * @param img
     * @return
     * @throws Exception
     */
    private static byte[] parseImg(File img) throws Exception {
        BufferedImage image = ImageIO.read(img);
        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        for (int y = 0; y < 200; y++) {
            for (int x = 0; x < 50; x++) {
                byte b = 0;
                for (int a = 0; a < 8; a++) {
                    b <<= 1;
                    b |= ((image.getRGB((x * 8) + a, y) & 0x00808080) == 0x00808080) ? 1 : 0;
                }
                outputStream.write(b);
            }
        }
        return outputStream.toByteArray();
    }

}
