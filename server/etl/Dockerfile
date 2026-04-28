FROM python:3.11-slim

WORKDIR /app

RUN pip install psycopg2-binary

COPY app.py .
COPY schema_dw.sql .

CMD ["python", "app.py"]