.PHONY: clean install test dep 

help:
	@echo "Please use \`make <target>' where <target> is one of"
	@echo "  clean		remove build products"
	@echo "  install	make and install"
	@echo "  test		run tests on installed pillow"
	@echo "  dep		install dependencies"

install:
	python setup.py install

dep:
	pip install -r requirements.txt

clean:	
	python setup.py clean
	rm -r build || true

test:
	cd test && python PgSQLTestCases.py

regression:
	cd test/regression && python array.py
	cd test/regression && python pgconnection.py
	cd test/regression && python pgresult.py
	cd test/regression && python pgversion.py
	cd test/regression && python unicode_tests.py
