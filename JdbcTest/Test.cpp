#include <stdio.h>
#include <Connection.h>

main (int argc, char **argv)
{
	Connection *connection = NULL;

	try
		{
		connection = createConnection();
		Properties *properties = connection->allocProperties();
                properties->putValue ("user", "sysdba");
                properties->putValue ("password", "masterkey");
		connection->openDatabase ("employee.gdb", properties);
		properties->release();


		PreparedStatement *statement = connection->prepareStatement (
			"select first_name, last_name, emp_no from employee where first_name = ?");
		statement->setString (1, "Robert");

		ResultSet *resultSet = statement->executeQuery();

		while (resultSet->next())
			{
			const char *firstName = resultSet->getString ("first_name");
			const char *lastName = resultSet->getString (2);	// last name
			short empNo = resultSet->getShort (3);					// emp-no
			printf ("%.10s %.15s %d\n", firstName, lastName, empNo);
			}

		resultSet->release();
		statement->release();
		connection->release();
		}
	catch (SQLException& exception)
		{
		printf ("Query failed: %s\n", exception.getText());
		if (connection)
			connection->release();
		return 1;
		}

	return 0;
}
