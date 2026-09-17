FROM gcc:latest
WORKDIR /app
COPY . .
RUN make servidor
EXPOSE 8080
RUN mkdir -p arquivos
CMD ["./servidor"]
