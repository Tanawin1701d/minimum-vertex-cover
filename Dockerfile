FROM ubuntu:20.04

WORKDIR "/"

RUN apt-get update && apt-get install -y build-essential

ADD ./s128_final.cpp /s128_final_at_docker.cpp

RUN g++ -fopenmp -o prog s128_final_at_docker.cpp

ENV PATH=$PATH:.

#sudo time docker run -v /media/tanawin/Backup\ Plus/Learning/Hpa/ass8_proj/input/:/input  -v /media/tanawin/Backup\ Plus/Learning/Hpa/ass8_proj/output/:/output tanawin1701d/sdgsdrtfrgs5sdrt5rdt  prog /input/ring-100-100 /output/ring-100-100.out
