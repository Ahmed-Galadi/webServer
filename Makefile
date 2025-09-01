# ============ Makefile ============
NAME = webserv
CXX = g++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -g -MMD 

SRCDIR = src
INCDIR = include
OBJDIR = obj

SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/server/Server.cpp \
          $(SRCDIR)/server/EventManager.cpp \
          $(SRCDIR)/config/ServerConfig.cpp \
          $(SRCDIR)/config/LocationConfig.cpp \
          $(SRCDIR)/config/ParseUtils.cpp \
          $(SRCDIR)/config/ParsingBlock.cpp \
          $(SRCDIR)/config/ConfigParser.cpp \
          $(SRCDIR)/client/Client.cpp \
          $(SRCDIR)/http/requestParse/Request.cpp \
		  $(SRCDIR)/http/requestParse/RequestBody.cpp \
		  $(SRCDIR)/http/requestParse/RequestParser.cpp \
          $(SRCDIR)/http/response/Response.cpp \
		  $(SRCDIR)/http/response/HttpMethodhandler.cpp
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEPFILES = $(OBJECTS:.o=.d)

all: $(NAME)

$(NAME): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

# Development targets
test: $(NAME)
	./$(NAME) config/default.conf

debug: CXXFLAGS += -DDEBUG
debug: re

# Include dependency files if they exist
-include $(DEPFILES)
