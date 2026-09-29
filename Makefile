PREFIX ?= /usr
DESTDIR ?=

BINDIR ?= $(PREFIX)/bin
SYSTEMD_USER_DIR ?= $(PREFIX)/lib/systemd/user

CC ?= gcc
PKG_CONFIG ?= pkg-config

REQUIRED_PACKAGES = gtk+-3.0 libnotify libsystemd
PKG_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(REQUIRED_PACKAGES) 2>/dev/null)
PKG_LIBS := $(shell $(PKG_CONFIG) --libs $(REQUIRED_PACKAGES) 2>/dev/null)

override CFLAGS += -Wall -Wextra $(PKG_CFLAGS)

SRC_DIR = src
TEST_DIR = tests
PROJECT_CPPFLAGS = -I$(SRC_DIR)

TARGET = eyeki
SOURCE_NAMES = eyeki.c activity.c activity_selection.c config.c config_watch.c \
	lifecycle.c presentation.c runtime.c scheduler.c
HEADER_NAMES = activity.h activity_selection.h config.h config_watch.h \
	lifecycle.h presentation.h runtime.h scheduler.h version.h
SOURCES = $(addprefix $(SRC_DIR)/,$(SOURCE_NAMES))
HEADERS = $(addprefix $(SRC_DIR)/,$(HEADER_NAMES))
SERVICE = eyeki.service

TEST_TARGETS = $(TEST_DIR)/test_scheduler $(TEST_DIR)/test_config \
	$(TEST_DIR)/test_config_watch $(TEST_DIR)/test_runtime \
	$(TEST_DIR)/test_activity_selection $(TEST_DIR)/test_presentation \
	$(TEST_DIR)/test_lifecycle

all: $(TARGET)

check-deps:
	@if ! $(PKG_CONFIG) --exists $(REQUIRED_PACKAGES); then \
		echo "Missing EyeKi build dependencies." >&2; \
		$(PKG_CONFIG) --print-errors --exists $(REQUIRED_PACKAGES) 2>&1 || true; \
		echo "Ubuntu 24.04: sudo apt-get install build-essential pkg-config libgtk-3-dev libnotify-dev libsystemd-dev" >&2; \
		exit 1; \
	fi

$(TARGET): $(SOURCES) $(HEADERS) | check-deps
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ \
		$(SOURCES) $(PKG_LIBS)

install: $(TARGET)
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -Dm644 $(SERVICE) $(DESTDIR)$(SYSTEMD_USER_DIR)/$(SERVICE)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(SYSTEMD_USER_DIR)/$(SERVICE)

clean:
	rm -f $(TARGET) $(TEST_TARGETS)

test: $(TEST_TARGETS)
	./$(TEST_DIR)/test_scheduler
	./$(TEST_DIR)/test_config
	./$(TEST_DIR)/test_config_watch
	./$(TEST_DIR)/test_runtime
	./$(TEST_DIR)/test_activity_selection
	./$(TEST_DIR)/test_presentation
	./$(TEST_DIR)/test_lifecycle

$(TEST_DIR)/test_scheduler: $(TEST_DIR)/test_scheduler.c \
		$(SRC_DIR)/scheduler.c $(SRC_DIR)/scheduler.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) -o $@ \
		$(TEST_DIR)/test_scheduler.c $(SRC_DIR)/scheduler.c

$(TEST_DIR)/test_config: $(TEST_DIR)/test_config.c $(SRC_DIR)/config.c \
		$(SRC_DIR)/config.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) -o $@ \
		$(TEST_DIR)/test_config.c $(SRC_DIR)/config.c

$(TEST_DIR)/test_config_watch: $(TEST_DIR)/test_config_watch.c \
		$(SRC_DIR)/config.c $(SRC_DIR)/config.h \
		$(SRC_DIR)/config_watch.c $(SRC_DIR)/config_watch.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) -o $@ \
		$(TEST_DIR)/test_config_watch.c $(SRC_DIR)/config.c \
		$(SRC_DIR)/config_watch.c

$(TEST_DIR)/test_runtime: $(TEST_DIR)/test_runtime.c \
		$(SRC_DIR)/runtime.c $(SRC_DIR)/runtime.h $(SRC_DIR)/config.c \
		$(SRC_DIR)/config.h $(SRC_DIR)/scheduler.c $(SRC_DIR)/scheduler.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) -o $@ \
		$(TEST_DIR)/test_runtime.c $(SRC_DIR)/runtime.c \
		$(SRC_DIR)/config.c $(SRC_DIR)/scheduler.c

$(TEST_DIR)/test_activity_selection: $(TEST_DIR)/test_activity_selection.c \
		$(SRC_DIR)/activity_selection.c $(SRC_DIR)/activity_selection.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) -o $@ \
		$(TEST_DIR)/test_activity_selection.c \
		$(SRC_DIR)/activity_selection.c

$(TEST_DIR)/test_presentation: $(TEST_DIR)/test_presentation.c \
		$(SRC_DIR)/presentation.c $(SRC_DIR)/presentation.h \
		$(SRC_DIR)/config.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) -o $@ \
		$(TEST_DIR)/test_presentation.c $(SRC_DIR)/presentation.c

$(TEST_DIR)/test_lifecycle: $(TEST_DIR)/test_lifecycle.c \
		$(SRC_DIR)/lifecycle.c $(SRC_DIR)/lifecycle.h
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) -o $@ \
		$(TEST_DIR)/test_lifecycle.c $(SRC_DIR)/lifecycle.c

.PHONY: all check-deps install uninstall clean test
