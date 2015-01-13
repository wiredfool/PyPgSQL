
import os

class Defaults:
	host = os.environ.get('PG_HOST','/var/run/postgresql')
	port = os.environ.get('PG_PORT','5432')
