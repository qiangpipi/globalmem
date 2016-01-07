####Globalmem
```
make
sudo su - root
insmod globalmem.ko
mknod /dev/globalmem c 200 0
echo 'hello world' > /dev/globalmem
cat /dev/globalmem
```
echo command write chars to the device;
cat command read chars from device;
Check the syslog while write/read the device; KERN_INFO will output to it.
