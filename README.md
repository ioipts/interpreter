# Simple Interpreter
  
อินเทอร์พรีเตอร์ แบบง่าย เขียนขึ้นมาใน 2 ภาษา ได้แก่  
standard C สำหรับ IoT  
Javascript  สำหรับ web browser  
เพื่อใช้ในการศึกษาและการใช้งานพัฒนาโปรแกรมบนอุปกรณ์ IoT ที่ง่ายขึ้น  
  
โค้ดประกอบด้วย 2 ส่วน  
  
Parser(พาร์เซอร์ หรือ โปรแกรมแปลงภาษา) ทำหน้าที่อ่านโค้ส Python หรือภาษาอื่นๆ เพื่อแปลงเป็น ชุดคำสั่ง (bytecode) และส่งให้อินเทอร์พรีเตอร์รันคำสั่งต่อไป  
ในขณะนี้สามารถใช้ได้สำหรับภาษา Python  
  
Interpreter(อินเทอร์พรีเตอร์ หรือ โปรแกรมรันชุดคำสั่ง) ทำหน้าที่รันชุดคำสั่ง (bytecode) ที่ได้จาก พาร์เซอร์ เพื่อทำให้อุปกรณ์ IoT หรือ web browser หรือคอมพิวเตอร์ทำงานตามโค้สนั้นๆ  

## Web site
http://code.mekmok.com  

## Language support  
Python  
  
## How to run on device
cd sample  
make  
./testinterpreter test/def.py ../web/test/def.bin  
  
## How to run on web browser
deploy web/ on apache, nginx  
browser to web/test.html  

# Author
- Pit Suwongs พิทย์ สุวงศ์ (admin@ornpit.com)  