#!/usr/bin/env python3
from gql import gql, Client 
from gql.dsl import DSLQuery, DSLSchema, dsl_gql
from gql.transport.requests import RequestsHTTPTransport
import json
import psutil
import time
import datetime
import fire

def FakeBoSL(name:str="FakeBoSL0"):
    # Select your transport with a defined url endpoint
    transport = RequestsHTTPTransport(url="https://cms.leigh.sh/graphql",headers={'Authorization':'Bearer nNgr-OJA-K2cNLkfZWQ0B-Xzlrkb9coN'},verify=True,retries=3)

    # Create a GraphQL client using the defined transport
    client = Client(transport=transport, fetch_schema_from_transport=True)

    # Using `with` on the sync client will start a connection on the transport
    # and provide a `session` variable to execute queries on this connection.
    # Because we requested to fetch the schema from the transport,
    # GQL will fetch the schema just after the establishment of the first session
    with client as session:

        # We should have received the schema now that the session is established
        assert client.schema is not None

        def fake_bosl_data():
            data = {}
            data["bosl_name"] = ""
            data["bosl_imei"] = "123456789ABCDEF"
            ts = datetime.datetime.fromtimestamp(psutil.boot_time()).astimezone().isoformat()
            data["bosl_bootup_timestamp"] = f'"{ts}"'
            data["bosl_battery_mv"] = 4200
            data["chmln_top_raw_a"] = 300
            data["chmln_top_raw_b"] = 300
            data["chmln_top_raw_avg"] = 300
            data["chmln_top_raw_resistance"] = 300

            data["chmln_bot_raw_a"] = 300
            data["chmln_bot_raw_b"] = 300
            data["chmln_bot_raw_avg"] = 300
            data["chmln_bot_raw_resistance"] = 300

            #data["ds18b20_top_temperature"] = 22
            data["ds18b20_bot_temperature"] = 21
            data["smt100_top_vwc"] = 80
            data["smt100_top_temperature"] = 21


            return data

        # Manual GQL WRITE
        query = gql(
                """
                mutation {
                    create_column_sensors_item(data: """ + str(fake_bosl_data()).replace("'","") + """){
                        id
                    }
                }
                """
                )

        result = session.execute(query)
        time.sleep(1)

if __name__ == "__main__":
    fire.Fire(FakeBoSL)
