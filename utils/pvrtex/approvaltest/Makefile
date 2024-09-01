# Variables
SOURCEIMAGE = crate.png
TEST_DIR = tests
APPROVED_DIR = $(TEST_DIR)/approved
RECEIVED_DIR = $(TEST_DIR)/received
RUN_DIR = $(TEST_DIR)/run
PVRTEX = ../pvrtex

# Default target
all: test

# Define the array of commands
TESTS = \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.dt" \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.vq" \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.tex" \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.dt -f argb4444 -d -c 64 -m quality -r -R" \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.dt -f normal -m" \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.dt -s" \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.dt -f pal8bpp -C 64 -d" \
	" -i $(SOURCEIMAGE) -o $(RUN_DIR)/texture.dt -f pal4bpp -d -p $(RUN_DIR)/preview.png"	

# Target to run all tests
received:
	@mkdir -p $(RUN_DIR)
	@for cmd in $(TESTS); do \
		rm -f $(RUN_DIR)/*; \
		$(PVRTEX) $$cmd; \
		TESTNAME=$$(echo $$cmd | sed -e 's/[^A-Za-z0-9_-]/_/g'); \
		mkdir -p $(RECEIVED_DIR)/$$TESTNAME; \
		mv $(RUN_DIR)/* $(RECEIVED_DIR)/$$TESTNAME; \
	done

.PHONY: received

compare: received
	@for dir in $(RECEIVED_DIR)/*; do \
		if [ -d "$$dir" ]; then \
			echo "--------------------------------------------------"; \
			echo "\nComparing test in output in: $$dir"; \
			for file in $$dir/*; do \
				echo "comparing output received: $$(basename $$file) with approved"; \
				cmp $$file $(APPROVED_DIR)/$$(basename $$dir)/$$(basename $$file); \
				if [ $$? -ne 0 ]; then \
					echo "Error: comparing $$file with approved"; \
					exit 1; \
				fi; \
			done; \
		fi; \
	done
	@echo "\nAll tests passed!"; \

# Approve results
approve:
	rm -rf $(APPROVED_DIR)
	mv $(RECEIVED_DIR) $(APPROVED_DIR)

# Clean up
clean:
	@rm -rf $(RECEIVED_DIR)

.PHONY: all test approve clean