copy PREDICTIVEMAINT.readings (
    id filler int,
        asset_id,
        unixtimestamp,
        asset_status ,
        temperature_sensor,
        temperature ,
        powerfactor_sensor,
        powerfactor,
        airflow_sensor,
        airflow,
        pressure_sensor,
        pressure,
        vibration_sensor,
        vibration,
        maintenance,
        batch_id
)
from  '\Users\connie-knchong\Downloads\Test_Data.csv'  delimiter ',' null ''  enclosed by '"';
