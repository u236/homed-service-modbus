FROM    127.0.0.1:5000/homed-service:latest
ARG     TARGETARCH
RUN     mkdir /etc/homed
RUN     ln -s /data/homed-modbus.conf /etc/homed/homed-modbus.conf
RUN     ln -s /data/homed.log /var/log/homed.log
ADD     share /usr/share
COPY    homed-modbus-$TARGETARCH /usr/bin/homed-modbus
CMD     ["/usr/bin/homed-modbus"]
