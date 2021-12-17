# CN466Project

# Sensor ตรวจจับจำนวนการเปิด/ปิดลิ้นชักในแต่ละวัน และตรวจจับความชื้นปัจจุบันภายในลิ้นชัก

จัดทำโดย: นายธีรัช ประสิทธิ์เวช 6110613178

Motivation
โปรเจคนี้เป็นโปรเจคเกี่ยวกับการทำเซนเซอร์ตรวจจับจำนวนการเปิด/ปิดลิ้นชักในแต่ละวัน และตรวจจับความชื้นปัจจุบันภายในลิ้นชัก เนื่องจากลิ้นชักเป็นเฟอร์นิเจอร์ที่สามารถ
ชำรุดได้จากการเปิด/ปิดลิ้นชักหลายครั้ง ทำให้การรับรู้จำนวนการเปิด/ปิดลิ้นชักเป็นประโยชน์ต่อการคาดเดาอายุการใช้งานของตัวลิ้นชัก และวางแผนถนอมการใช้งานได้ง่ายขึ้น
ขณะที่การรับรู้ความชื้นปัจจุบันภายในลิ้นชัก จะช่วยให้การถนอมสิ่งของที่ความชื้นมีผลต่อคุณภาพ เป็นไปอย่างง่ายดายขึ้น

Requirement
- บอร์ด Cucumber RS: ใช้เซนเซอร์วัดความเร่งและความชื้น เซนเซอร์วัดความเร่งรายงานค่าทุกครั้งที่มีการเปิดหรือปิดลิ้นชัก ขณะที่เซนเซอร์วัดความชื้นรายงานค่า
ทุก 15 วินาที
- HiveMQ: topic ของเซนเซอร์วัดความเร่งคือ cn466/drawer/cucumber_tar, topic ของเซนเซอร์วัดความชื้อคือ
cn466/drawer_humid/cucumber_tar, paylord มี status และ humidity
- LINE bot: รองรับ text message
- Edge Impulse: ตรวจจับความเร่งเพื่อตรวจสอบการขยับแต่ละครั้งของลิ้นชัก
- LIFF: UI component เป็นการแสดงหน้าเว็บไซต์แบบ tall โดยหน้าเว็บไซต์สามารถเลือกวันที่ที่จะดูประวัติจำนวนการเปิด/ปิดลิ้นชักของวันนั้นๆ
แทนการพิมพ์ได้
- Web API: Web Animation API ทำหน้าที่แสดงภาพเคลื่อนไหวเป็นภาพพื้นหลังในหน้าเว็บไซต์ที่เข้าถึงผ่าน LIFF ได้
- Cloud service: Firebase ทำหน้าที่บันทึกจำนวนการขยับลิ้นชักในแต่ละวัน
- Project server: สั่งให้โปรเจคทำงานบน Heroku

Design
1. สร้าง Machine Learning ด้วย Edge Impulse และรับค่าความเร่งผ่านเซนเซอร์วัดความเร่ง โดยใช้ค่าความเร่งในการบ่งบอกว่าขณะนี้ลิ้นชักกำลังขยับอยู่
หรือไม่ได้ขยับ
2. import Machine Learning จาก Edge Impulse เป็นไฟล์ arduino แก้ไขโค้ดในไฟล์ให้ส่งค่า status เป็นการบ่งบอกว่าลิ้นชักมีการ
ขยับเกิดขึ้น ส่งทุกครั้งที่มีการขยับลิ้นชัก และส่งค่า humidity เป็นค่าความชื้นในปัจจุบัน ส่งทุก 15 วินาที ไปยัง MQTT HiveMQ โดยค่า status ส่งไปยัง
topic cn466/drawer/cucumber_tar ส่วนค่า humidity ส่งไปยัง topic cn466/drawer_humid/cucumber_tar
3. สร้าง LINE bot ใน LINE Developers เขียนโค้ดควบคุมการทำงานของ LINE bot ให้รับค่า status และ humidity จาก MQTT โดย
ทุกครั้งที่มีการรับค่า status เข้ามา 2 ครั้ง ให้นับว่ามีการเปิด/ปิดลิ้นชักจำนวน 1 ครั้ง
4. สร้างโปรเจค Firebase เพื่อเก็บค่า วัน/เดือน/ปี ที่มีการเปิด/ปิดลิ้นชัก และจำนวนการเปิด/ปิดลิ้นชักของวันนั้นๆ เขียนโค้ด LINE bot ให้ทุกครั้งที่มีการรับค่า
status จะบันทึกวัน/เดือน/ปี และจำนวนการเปิด/ปิดลิ้นชักปัจจุบันในวันนั้น ลงบน Firebase
5. กำหนดให้มีการตอบกลับ text message ประกอบไปด้วย
1) จำนวนการเปิด/ปิดลิ้นชักในวันนั้น
2) ค่าความชื้นในปัจจุบัน
3) เลือก วัน/เดือน/ปี ที่ต้องการดูประวัติการเปิด/ปิดลิ้นชัก
4) ประวัติจำนวนการเปิด/ปิดลิ้นชักตาม วัน/เดือน/ปี ที่กำหนด
5) วัน/เดือน/ปี ปัจจุบัน
6) วัน/เดือน/ปี เลือกไว้เพื่อดูประวัติการเปิด/ปิดลิ้นชัก
7) แสดงข้อมูลโปรไฟล์ของผู้ใช้
6. สร้างหน้าเว็บไซต์เป็นไฟล์ html ที่สามารถเลือก วัน/เดือน/ปี ที่ต้องการดูประวัติการเปิด/ปิดลิ้นชักได้ เมื่อเลือกแล้วจะทำการพิมพ์ text message ตามรูปแบบ
ที่กำหนดให้โดยอัติโนมัติ พื้นหลังของเว็บไซต์ทำเป็นภาพเคลื่อนไหวด้วย Web Animation API
7. สร้าง LINE LIFF ใน LINE Developers กำหนดให้สามารถทำการกดเพื่อลิงค์ไปยังหน้าเว็บไซต์ที่สร้างไว้ได้ ปรับแต่งรูปแบบของ LINE LIFF ด้วย
LINE Rich Menu
8. สร้างโปรเจค Heroku และอัปโหลดโค้ดโปรเจค LINE bot ไปยัง Heroku

Test
1. รับ text message "status" จะตอบกลับเป็นข้อความ "จำนวนการเปิด/ปิดลิ้นชักในวันนี้: <จำนวนการเปิด/ปิดลิ้นชักในวันนั้น>"
2. รับ text message "humidity" จะตอบกลับเป็นข้อความ "ความชื้นปัจจุบัน: <ความชื้นปัจจุบัน>"
3. รับ text message "<วันที่>/<เดือน>/<ปี>" จะตอบกลับเป็นข้อความ "วันที่เลือก: <วัน/เดือน/ปี ที่เลือก>"
4. รับ text message "selected status" จะตอบกลับเป็นข้อความ
"วันที่เลือก: <วัน/เดือน/ปี ที่เลือก>
<จำนวนการเปิด/ปิดลิ้นชัก ของวันที่เลือก>"
5. รับ text message "date" จะตอบกลับเป็นข้อความ "วันนี้: <วัน/เดือน/ปี ปัจจุบัน>"
6. รับ text message "selected date" จะตอบกลับเป็นข้อความ "วันที่เลือก: <วัน/เดือน/ปี ที่เลือก>"
7. รับ text message "profile" จะตอบกลับเป็นข้อความ
"Display name: <ชื่อ display ของผู้ใช้>
Profile image: "
และรูปภาพโปรไฟล์ของผู้ใช้
8. กด LINE LIFF แล้วลิงค์ไปยังหน้าเว็บไซต์สำหรับเลือกวันที่ แสดงเว็บไซต์แบบ tall พื้นหลังหน้าเว็บไซต์เป็นภาพเคลื่อนไหว มีช่องข้อความให้เลือกวันที่
และปุ่ม Ready
9. กดช่องข้อความให้เลือกวันที่ แล้วแสดงปฏิทินแบบ date picker กดเพื่อเลือก วัน/เดือน/ปี ได้
10. กดปุ่ม Ready แล้วมีการพิมพ์ text message เป็น "<วัน/เดือน/ปี ที่เลือก>" หลังกด ปุ่ม Ready เปลี่ยนเป็นปุ่ม Sent ทำหน้าที่เหมือนปุ่ม Ready
ทุกประการ