make
insmod q2.ko
cat -b /proc/hw1
read -p "press"&
echo E > /proc/hw1&
cat -b /proc/hw1&
echo G > /proc/hw1&
cat -b /proc/hw1&
echo E > /proc/hw1&
read -p "press"&
cat -b /proc/hw1&
rmmod q2.ko
